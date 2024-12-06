#pragma once

#include "nocopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class EventLoop;
class Channel;
class Socket;
/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * =>  打包TcpConnection 设置回调 => Channel => Poller => Channel的回调操作
 */

class TcpConnection : nocopyable , public std::enable_shared_from_this<TcpConnection>//等到当前对象的智能指针
{
public:
    TcpConnection(EventLoop* loop,
                const std::string &name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);

    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_;}
    const InetAddress& localAddress() const { return localAddr_;}
    const InetAddress& peerAddress() const { return peerAddr_;}

    bool connected() const { return state_==kConnected; }

    //发送数据(给客户端发送)
    void send(const std::string & buf);

    //关闭连接(服务端关闭)
    void shutdown();

    //设置回调
    void setConnectionCallback(const ConnectionCallback & cb){ connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback & cb) {messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback & cb) { writeCompleteCallback_=cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    void setCloseCallback(const CloseCallback & cb ) { closeCallback_ = cb;}

    //连接建立
    void connectEstablished();

    //连接销毁
    void connectDestroy();
private:
    enum stateE {kDisconnected,//已经断开连接
                kConnecting, //正在连接
                kConnected, //已经连接
                kDisconnecting //正在断开连接
                };

    void setState(stateE state) { state_=state;}

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message , size_t len);
    void shutdownInloop();

    EventLoop* loop_;//这里绝对不是baseloop，因为TcpConnection都是在subloop里面管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    //这里和Acceptor类似 Acceptor=>mainloop  TcpConnection=>subloop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_ ;//有连接时的回调
    MessageCallback messageCallback_ ; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_ ; //消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_ ; //消息达到一定数量的标记  保持发送发和接收方 速率保持一致 
    CloseCallback closeCallback_ ; 
    size_t highWaterMark_;

    Buffer inputBuffer_; // 接收数据的缓冲区(指服务端接受)
    Buffer outputBuffer_; // 发送数据的缓冲区(指服务端发送)
};