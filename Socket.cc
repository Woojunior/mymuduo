#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
Socket::~Socket()
{
    close(sockfd_);//关闭文件描述，清空其资源
}

//命名sockfd，将存有ip和port的地址和sockfd绑定
void Socket::bindAddress(const InetAddress & localaddr)
{   
    if(0!=::bind(sockfd_,(sockaddr*) localaddr.getSockAddr() , sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail \n" , sockfd_);
    }
}

//指定监听的sockfd 和监听队列的长度
void Socket::listen()
{
    if(0!=::listen(sockfd_,1024)){
        LOG_FATAL("listen sockfd:%d fail \n" , sockfd_);
    }
}

//执行过监听的sockfd， peeraddr能获取远端（客户端）的地址
int Socket::accept(InetAddress* peeraddr)
{   
    /**
     * 1.accept函数参数不合法
     * 2.对返回connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     *  poller + non-block iO
     */
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr , sizeof addr);//清空地址
    int connfd = ::accept4(sockfd_,(sockaddr*) &addr , &len,SOCK_CLOEXEC|SOCK_NONBLOCK);//将远端的地址(客户端地址)存在addr中
    if(connfd>= 0)
    {
        peeraddr->setSockAddr(addr);//将远端的地址给 这个函数调用传入的 参数
    }
    return connfd;//函数返回新连接的sockfd
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR)<0){
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval=on ? 1 : 0;
    //TCP选项 禁止Nagle算法 即禁止将小的数据包合并成较大的数据包以减少网络负载的技术，以便降低延迟
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval, sizeof optval); 
}

void Socket::setReuseAddr(bool on)
{
    int optval=on ? 1 : 0;
    //通用socket选项 重用本地地址 它允许多个套接字（进程或线程）绑定到相同的本地地址和端口上，或允许某个套接字在上一个套接字关闭后立即重新绑定到相同的端口
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval, sizeof optval); 
}

void Socket::setReusePort(bool on)
{
    int optval=on ? 1 : 0;
    //通用socket选项 它允许多个套接字（通常是多个进程或线程）绑定到同一个端口号上
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval, sizeof optval); 
}

void Socket::setKeepAlive(bool on)
{
    int optval=on ? 1 : 0;
    //通用socket选项 发送周期性活报文 以维持连接
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval, sizeof optval); 
}