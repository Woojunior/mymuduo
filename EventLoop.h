#pragma once

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>


#include"nocopyable.h"
#include"Timestamp.h"
#include"CurrentThread.h"

class Channel;
class Poller;

class EventLoop : nocopyable
{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //推出事件循环
    void quit();

    //记录Poller返回的时间
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //把cb放入队列，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);
    
    //用来唤醒loop所在的线程
    void wakeup();

    //EventLoop的方法=》Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断Eventloop对象是否在自己的线程里面
    bool isInLoopThread() const{ return threadId_== CurrentThread::tid();}
private:
    void handleRead();//wake up
    void doPendingFunctors();//执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;//原子操作，通过CAS实现的
    std::atomic_bool quit_;//标识退出loop事件

    const pid_t threadId_;//记录当前loop所在线程的id
    Timestamp pollReturnTime_;//poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//主要作用，当前mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;//eventloop管理的所有的channel
    //Channel* currentActiveChannel_;//指针做一些断言操作

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否需要执行回调操作
    std::vector<Functor> pendingFunctors_;//储存loop需要执行的所有回调操作
    std::mutex mutex_;//互斥锁，用户保护上面vector容器的线程安全操作
};