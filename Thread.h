#pragma once
#include"nocopyable.h"

#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>
class Thread : nocopyable
{
public:
    using ThreadFunc = std::function<void()>; //线程函数的函数类型，如果想有参数使用bind绑定器
    //构造和析构
    explicit Thread(ThreadFunc, const std::string & name = std::string());
    ~Thread();

    void start();
    void join();

    //返回私有变量的值接口
    bool started() const {return started_;}
    pid_t tid() const {return tid_;}
    const std::string& name() const {return name_;}
    static int numCreated() { return numCreated_; }
private:
    void setDefaultName(); //设置默认的线程名

    bool started_;
    bool joined_; //当前线程等待其他线程运行完了，继续往下运行
    //std::thread thread_;//这里不能直接创建线程对象，因为直接创建线程就启动了，无法管理
    std::shared_ptr<std::thread> thread_; //所以这里使用智能指针进行管理
    pid_t tid_; //记录线程的id
    ThreadFunc func_; //存储线程函数的
    std::string name_; //每个线程都有一个名字
    static std::atomic_int numCreated_; //对所有线程进行计数，所以使用静态的
};