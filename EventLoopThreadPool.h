#pragma once
#include "nocopyable.h"

#include<functional>
#include<string>
#include<vector>
#include<memory>

class EventLoop;
class EventLoopThread;
class EventLoopThreadPool : nocopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop * baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    //设置底层线程的数量
    void setThreadNum(int numThreads) { numThreads_=numThreads;}

    //根据上面指定的数量，在 池 里面创建事件线程
    void start(const ThreadInitCallback & cb = ThreadInitCallback());

    //如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    EventLoop* baseLoop_; // 用户最初创建的一个事件循环
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;//做轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //创建一个装有EventLoopThread智能指针的数组,用智能指针存储不想手动的释放资源
    std::vector<EventLoop*> loops_; //创建一个装有EventLoop指针的数组 通过调用EventLoopThread中的startLoop()得到EventLoop*
};