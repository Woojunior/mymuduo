#pragma once

#include"nocopyable.h"

class InetAddress;

//封装socket fd
class Socket : nocopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd){}
    ~Socket();

    int fd() const {return sockfd_;}//获得sockfd的接口

    //命名sockfd_，将存有ip和port的地址和sockfd绑定
    void bindAddress(const InetAddress & localaddr);

    //指定监听的sockfd_ 和监听队列的长度
    void listen();

    //执行过监听的sockfd_，才能执行accept， peeraddr能获取远端（客户端）的地址
    int accept(InetAddress* peeraddr);

    //关闭sockfd_上的写
    void shutdownWrite();

    //设置sockfd的属性
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);


private:
    const int sockfd_;

};