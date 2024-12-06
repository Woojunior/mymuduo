#include "TcpConnection.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>
#include <errno.h>
#include <string>


//检查指针baseloop是否为空，为空则终止
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n" , __FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop,sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) //64MB
{
    //下面给channel 设置相应的回调函数，poller给Channel通知感兴趣的事件发生了，Channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1)
    );

    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this)
    );

    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this)
    );

    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this)
    );
    
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n" , name_.c_str() , sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

//发送数据(给客户端发送)
void TcpConnection::send(const std::string &buf)
{
    if(state_== kConnected)//已连接的状态
    {
        if(loop_->isInLoopThread())//判断是否在loop的当前线程发送
        {
            sendInLoop(buf.c_str(),buf.size());
        }
        else    //如果不是在当前线程，则唤醒所在线程进行发送
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
    

}

/**
 * 发送数据 应用写的快，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void* data , size_t len)
{
    ssize_t nwrote=0;//已经写入发送缓存区多少
    size_t remaining = len;//还剩下多少没写入，因此初始时数据的原始长度
    bool faultError = false;//定义出错的标志

    //之前调用过 connection 的shutdown，不能再进行发送了
    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing !");
        return;
    }

    //表示channel_ 第一次 开始写数据， 而且发送缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(),data,len);
        if(nwrote>0) //写入数据成功
        {
            remaining=len-nwrote;//得到 还剩多少数据没有写入发送缓冲区
            if(remaining==0 && writeCompleteCallback_)//如果数据全部写入，且 写完成的回调已经有了
            {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_,shared_from_this())
                );
            }
        }
        else //n<0的情况 写入发送缓冲区失败
        {
            nwrote=0;
            /**
             * 表示操作是非阻塞的，但是当前没有数据可以读取或写入。
             * 在非阻塞模式下，这个错误通常是可以忽略的，因为它只是表示当前没有数据，不代表出现了严重的错误
             */
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                /**
                 * EPIPE：表示写操作尝试向一个已经关闭的连接写数据（比如对端已经关闭了连接，导致无法写入）。
                 * ECONNRESET：表示连接被重置，通常是对端主机突然断开连接或发生其他网络故障
                 */
                if(errno == EPIPE || errno==ECONNRESET) //SIGPIPE RESET
                {
                    faultError=true;
                }
            }
        }
    }
    // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock也就是channel，调用writeCallback_回调方法
    // 也就是调用TcpConnection::handleWrite()方法，把发送缓冲区中全部发送完成
    if(!faultError && remaining>0)
    {
        //目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();

        //缓冲区剩余待发送的数据 和 剩余的数据 总和 超过了水位线
        //没加剩余数据前，原来缓冲的数据并没有超过水位线
        //且超过水位线的回调函数 已经有了定义
        if(oldLen + remaining >= highWaterMark_ 
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining)
            );
        }
        outputBuffer_.append((char*)data+nwrote , remaining); //将剩下的数据加入到发送缓冲区

        if(!channel_->isWriting()) //如果这个channel_不是可写的
        {
            channel_->enableWriting(); //这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
    }
}


//关闭连接
void TcpConnection::shutdown()
{
    if(state_==kConnected) //处于已连接的状态才要关闭
    {
        setState(kDisconnected);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInloop,this)
        );
    }
}

void TcpConnection::shutdownInloop()
{
    if(!channel_->isWriting())  //说明 发送数据的缓冲区outBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite();//关闭写端
    }
}

//连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    //将channel_和其上层的TcpConnection进行绑定，防止channel在进行回调时TcpConnection对象被remove了，那再chaneel再调用相应的回调结果就是未知的
    //channel的回调 其实是 TcpConnection给它的
    channel_->tie(shared_from_this());

    channel_->enableReading(); //向Poller注册Channel的epollin事件

    // 连接建立，执行回调
    connectionCallback_(shared_from_this());
}

//连接销毁
void TcpConnection::connectDestroy()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();//把channel的所有感兴趣的事件，从poller中删掉
        connectionCallback_(shared_from_this());

    }

    channel_->remove();//把channel从poller中删掉
}



void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&savedErrno);

    if(n > 0)
    {
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }
    else if(n==0)
    {
        handleClose();//连接断开了
    }
    else{
        //n<0，发生了错误
        errno = savedErrno;
        LOG_ERROR("Tcpconnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting()) //如果是可写的
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), & savedErrno);
        if(n>0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes()==0)//当前没有可读的，代表数据发送完毕
            {
                channel_->disableWriting();//关闭写事件
                if(writeCompleteCallback_)
                {
                    //唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_,shared_from_this())
                    );
                }
                if(state_==kDisconnecting)//如果连接处于正在断开的状态
                {   
                    //关闭事件循环
                    shutdownInloop();
                }
            }
        }
        else 
        {
            //发生错误
            LOG_ERROR("Tcpconnection::handleWrite");
        }
    }
    else//并不是可写的
    {
        //如果写事件是关闭的
        LOG_ERROR("Tcpconnection fd=%d is done , no more writing \n", channel_->fd());
    }
}

//Poller => channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose()
{
    LOG_INFO("Tcpconnection::handleClose fd=%d state=%d \n" , channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this()); //指向本对象的智能指针
    connectionCallback_(connPtr);//执行连接关闭的回调
    closeCallback_(connPtr);//关闭连接的回调 执行的是TcpServer::removeConnection回调方法
    
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen=sizeof optval;
    int err=0;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen))
    {
        err=errno;
    }
    else
    {
        err=optval;
    }

    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(),err);
}