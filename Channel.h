#pragma once

#include"nocopyable.h"
#include"Timestamp.h"

#include<functional>
#include<memory>

class EventLoop;

// EventLoop Channel Poller 之间的关系 Reactor 模型
/*
    Channel理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN,EPOLLOUT事件
    还绑定了poller返回的具体事件

*/

//不允许拷贝和赋值
class Channel:nocopyable
{
public:
    //声明回调函数
    using EventCallback=std::function<void()>;
    using ReadEventCallback=std::function<void(Timestamp)>;//Timestamp可以设置超时时间
    //声明构造函数和析构函数
    Channel(EventLoop* loop , int fd);
    ~Channel();

    //fd得到poller通知后，处理事件相应的方法————回调
    void handleEvent(Timestamp receiveTime);

    // //设置回调函数对象
    void setReadCallback(ReadEventCallback cb){ readCallback_= std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_=std::move(cb);}

    //将channel_和其上层的TcpConnection进行绑定，防止channel在进行回调时TcpConnection对象被remove了，那再chaneel再调用相应的回调结果就是未知的
    //channel的回调 其实是 TcpConnection给它的
    void tie(const std::shared_ptr<void>&);

    //得到成员变量值的接口
    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) { revents_=revt;}//设置revents，因为实际发生的事件先由poller监听到fd上由事件发生，然后对其设置真是发生的事件

    //设置fd相应的事件状态
    void enableReading() {events_|=kReadEvent;update();}
    void disableReading() {events_&=~kReadEvent;update();}
    void enableWriting() {events_|=kWriteEvent;update();}
    void disableWriting() {events_&=~kWriteEvent;update();}
    void disableAll() {events_ = kNoneEvent; update();}

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() {return index_;}
    void set_index(int idx) { index_=idx;}
    //one loop per thread
    //一个线程有一个eventLoop，一个eventLoop里有一个poller，poller上可监听很多channel
    EventLoop* ownerLoop() {return loop_;}
    void remove();




private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    
    //fd状态 有没有注册感兴趣的事件
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;//事件循环
    const int fd_; //fd, Poller 监听的对象
    int events_; // 注册fd感兴趣的事件 如EPOLLIN EPOLLOUT
    int revents_; //poller 返回具体发生事件
    int index_;

    std::weak_ptr<void>  tie_;
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体事件revent,所以调用具体的事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;


};