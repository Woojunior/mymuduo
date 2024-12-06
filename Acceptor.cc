#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
//创建非阻塞的socket
static int createNonblocking()
{
    int sockfd=::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    if(socket<0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n" , __FILE__, __FUNCTION__ , __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop , const InetAddress & listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())//创建socket
    , acceptChannel_(loop_, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); //命名socket，给创建的socket 绑定含有 ip和port的 地址

    //TcpServer::start() Acceptor.listen 有新用户连接，要执行一个回调:(connfd->channel->subloop)
    //baseLoop监听到acceptChannel_(listenfd) 有事件发送，会调用channel的回调函数=>会调用acceptor预先设定好的handleRead()
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();//关闭所有事件的操作
    acceptChannel_.remove();//从EventLoop中移除当前的channel
}

void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen(); //在bind之后，开始监听
    acceptChannel_.enableReading(); //开启Channel上的读事件
}

//listenfd有事件发生了，即 有新用户连接 了
void Acceptor::handleRead()
{
    InetAddress peerAddr;//用来储存 用户的地址
    int connfd = acceptSocket_.accept(&peerAddr);//应该在listen执行之后
    if(connfd>=0)
    {
        //表示成功接受到一个新的连接，并且得到一个新的套接字connfd表示这个连接
        if(NewConnectionCallback_)
        {
            //执行回调
            NewConnectionCallback_(connfd,peerAddr);//轮询找到subloop，唤醒，分发当前的新客户端的channel

        }else{
            //没有定义回调
            ::close(connfd);
        }
    }else
    {
        //表示接受失败
        LOG_ERROR("%s:%s:%d accept error:%d \n", __FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE)
        {
            //表示资源描述符已经达到上限
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__,__FUNCTION__,__LINE__);
        }
    }

}