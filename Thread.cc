#include"Thread.h"
#include"CurrentThread.h"

#include<semaphore.h>

std::atomic_int Thread::numCreated_(0);
//构造和析构
Thread::Thread(ThreadFunc func, const std::string & name )
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(func)
    , name_(name)
{}

Thread::~Thread()
{
    if(started_ && !joined_)//如果线程启动，且线程不是工作线程
    {
        thread_->detach();//thread类提供的设置份力线程的方法，将其设置为守护线程，即主线程结束，守护线程自动结束
    }
}

//线程的启动
void Thread::start()    // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_=true;
    sem_t sem;
    //第二个参数表示这个信号量是局部的，不是进程之间共享的；第三个参数表示信号量初始化为0，即初始状态是“不可用”
    sem_init(&sem,false,0);
    
    //开启线程
    thread_=std::shared_ptr<std::thread>(new std::thread([&](){//  [&]() 表示该 lambda 捕获外部所有变量的引用
        //获取线程的tid值
        tid_=CurrentThread::tid();
        sem_post(&sem);// 通知主线程新线程已创建并准备好 V释放

        //开启一个新的线程，专门执行该线程函数
        func_();
    }));

    //这里主线程必须等待获取上面新创建线程的tid值
    sem_wait(&sem);//此时主线程阻塞在此  P等待 信号量大于零才返回
}
void Thread::join()
{
    joined_=true;
    thread_->join();
}

//设置默认的线程名 按创建的序号设置
void Thread::setDefaultName()
{
    int num=++numCreated_;

    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof buf , "Thread%d",num);
        name_=buf;
    }
}