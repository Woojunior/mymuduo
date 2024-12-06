#include"EventLoopThread.h"
#include"EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc,this) , name)//该线程的执行的 函数 和 线程名字
    , mutex_()
    , cond_()
    , callback_(cb)
{

}

EventLoopThread::~EventLoopThread()//线程退出
{
    exiting_=true;//表示退出循环

    if(loop_!=nullptr){
        loop_->quit();//退出线程绑定的事件循环
        thread_.join();//等待子线程结束
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start();//启动底层的新线程，执行的是EventLoopThread构造函数传给thread_的ThreadFunc回调函数，此函数在下面定义
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)//说明还没执行到EventLoopThread::threadFunc中的loop_=&loop;这个语句
        {
            cond_.wait(lock);//等待通知，拿到互斥锁
        }
        //得到loop，相当线程间的通信
        loop=loop_;
    }

    return loop;
}

// 下面这个方法，实在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop;//创建一个独立的EventLoop，和上面的线程是一一对应的，one loop per thread

    //根据构造函数中拿到的回调函数，对loop进行初始化
    if(callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;
        cond_.notify_one();//唤醒一个线程
    }

    loop.loop();//EventLoop loop => Poller.poll
    //事件循环将一直保持在等待活跃事件的状态，直至循环结束

    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;
}