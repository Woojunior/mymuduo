#pragma once
#include"nocopyable.h"
#include"Socket.h"
#include"Channel.h"

#include<functional>

class EventLoop;
class InetAddress;

class Acceptor:nocopyable
{
public:
    using NewConnectionCallback =std::function<void(int sockfd, const InetAddress&)>;
    //构造和析构
    Acceptor(EventLoop* loop , const InetAddress & listenAddr, bool reuseport);
    ~Acceptor();

    //设置回调函数的接口
    void setNewConnectionCallback( const NewConnectionCallback cb)
    {
        NewConnectionCallback_=cb;
    }

    bool listenning() const { return listenning_;}
    void listen();
private:
    void handleRead();


    EventLoop* loop_; // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    Socket acceptSocket_; //listenfd
    Channel acceptChannel_;//打包以上的 loop和aceeptSoket中的fd
    NewConnectionCallback NewConnectionCallback_;//用来将connfd 打包成channel，唤醒subloop，传给 subloop 来处理
    bool listenning_;//控制变量
};