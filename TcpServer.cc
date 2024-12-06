#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>


//检查指针baseloop是否为空，为空则终止
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n" , __FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string &nameArg,
                Option option)
                : loop_(CheckLoopNotNull(loop))
                , ipPort_(listenAddr.toIpPort())
                , name_(nameArg)
                , acceptor_(new Acceptor(loop,listenAddr,option==kReusePort))
                , threadPool_(new EventLoopThreadPool(loop,name_))
                , connectionCallback_()
                , messageCallback_()
                , nextConnId_(1)
                , started_(0)
{
    //当有新用户连接时，会执行TcpServer::newConnection 回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}

TcpServer::~TcpServer()
{
    //遍历Connections，将其释放掉
    for(auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);//这个局部的指针出了右括号 会被释放
        item.second.reset();//释放掉当前的智能指针

        //销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroy,conn)
        );
    }
}

//设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

//开启服务器监听 调用完start() 马上开始调用loop()即开启事件循环（Poller）
void TcpServer::start()
{
    if(started_++==0){//判断条件 防止重复开启
        threadPool_->start(threadInitCalllback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd,const InetAddress & peerAddr)
{   
    //轮询算法，选择一个subloop，来管理channel
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf , "-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName=name_+buf;

    LOG_INFO("TcpServer::newConnection [%s] - new Connection [%s] from %s \n",
        name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    //通过Sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;//用来储存本机的ip和端口信息
    ::bzero(&local,sizeof local);
    socklen_t addrlen= sizeof local;
    if(::getsockname(sockfd , (sockaddr*)&local , &addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local); 

    //根据连接成功的sockfd，创建TcpConnection连接对象 , 用一个智能指针指向这个对象
    TcpConnectionPtr conn(new TcpConnection(
                        ioLoop,//subloop
                        connName,
                        sockfd,
                        localAddr,
                        peerAddr
    ));
    // 将这个新的conn 储存到 保存所有的连接map中 《连接name，连接的智能指针》
    connections_[connName]=conn;

    //下面的回调都是用户设置给TcpServer=》TcpConnection=》Channel=》Poller=》notify channel 调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    //设置了如何关闭连接的回调 conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    //直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr & conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
        name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop * ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroy,conn)
    );
}