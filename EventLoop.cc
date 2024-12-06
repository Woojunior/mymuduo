#include"EventLoop.h"
#include"Logger.h"
#include"CurrentThread.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h> 
#include<mutex>
#include<memory>
//防止一个线程创建多个EventLoop  thread_local
__thread EventLoop* t_loopInThisThread = nullptr;

//定义默认的Poller IO复用接口的超时时间为10000ms
const int kPollTimeMS=10000;

//创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd=::eventfd(0, EFD_NONBLOCK| EFD_CLOEXEC);

    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }

    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))//获取默认的poller  我们用的是epoll
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
    //,currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n ",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    
    }else{
        t_loopInThisThread=this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));

    //每一个eventloop都将监听wakeupchannel的EPOLLIN事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while(!quit_)
    {
        activeChannels_.clear();//清空活跃(就绪)事件列表中的事件为下面接受做准备

        //监听两类fd  一种是client的fd 一种是wakeupfd
        pollReturnTime_=poller_->poll(kPollTimeMS,&activeChannels_);//通过poller得到活跃的事件表activeChannels_

        for(Channel* channel:activeChannels_){
            //Poller监听哪些channel发生了事件了，然后上报给EventLoop，然后EventLoop通知channel处理相应的事件(读写等)
            channel->handleEvent(pollReturnTime_);
        }

        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 mainLoop accept连接 得到fd封装在 subloop里的channel中
         * mainloop事先注册一个回调cb(需要subloop来执行)   wakeup subloop后，执行下面的方法(执行之前mainloop注册的cb操作)
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n" , this);
    looping_=false;
}
//退出事件循环
//1.loop在自己的线程中调用quit 2.在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_=true;

    if(!isInLoopThread()){
        //如果是在其他线程中，调用quit   在subloop（worker）中，调用了mainloop(IO)的quit
        wakeup();
    }

}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())//在当前线程中，执行cb
    {
        cb();
    }
    else //在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}
//把cb放入队列，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {   //传出这个作用域后 智能锁会自动释放
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //唤醒相应的 需要执行上面回调操作的loop线程了
    // ||callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调并没有执行
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();//唤醒loop所在的线程
    }
}

void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop:::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

//用来唤醒loop所在的线程的 向wakeuofd_写一个数据，wakeupChennel就发生读事件，当前loop线程就会被唤醒
//说白了就是让epoll_wait接受到数据，这样就代表这个线程被唤醒了,不是一直阻塞在poll()那个函数那里
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one, sizeof one);
    if(n!=sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

//Eventloop的方法调用 Poller的方法
//而Channel是通过EventLoop的方法间接地调用Poller的方法进行修改
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;//回调正在执行的标志

    {   //这个{}是智能锁的作用域 ，作用域结束自动解锁
        std::unique_lock<std::mutex> lock(mutex_);
        //这里为什么要与新创建的临时functors回调函数数组进行交换
        //是因为每时每刻都有很多回调操作加入到回调函数数组，这里交换了之后 真正储存回调函数的数组可以继续接受回调函数 临时的回调函数数组也可以处理
        //两者实现了并行
        functors.swap(pendingFunctors_);
    }

    for(const Functor & functor:functors)
    {
        functor();//执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_=false;//回调结束的标志
}

