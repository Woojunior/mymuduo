#pragma once

#include"nocopyable.h"
#include"Thread.h"

#include<functional>
#include<string>
#include<mutex>
#include<condition_variable>

class EventLoop;

//主要是实现 一个线程 绑定 一个loop
class EventLoopThread : nocopyable
{
public:
    using ThreadInitCallback =std::function<void(EventLoop*)>;//线程初始化的回调函数

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();//开启一个线程的事件循环
private:
    void threadFunc();//线程执行的函数

    EventLoop* loop_;
    bool exiting_;//表示是否退出循环
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;

};