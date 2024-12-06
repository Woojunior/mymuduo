/**
 * 用户使用muduo库编写服务器程序
 * 包含了头文件，使用户方便 只需要包含头文件TcpServer.h
 */
#pragma once
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "nocopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

//对外的服务器变成使用的类
class TcpServer : nocopyable
{
public:
    using ThreadInitCalllback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort);
    
    ~TcpServer();

    //设置回调
    void setThreadInitCallback(const ThreadInitCalllback & cb ) { threadInitCalllback_ = cb;}
    void setConnectionCallback(const ConnectionCallback & cb ) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback & cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback & cb) { writeCompleteCallback_ =cb; } 

    //设置底层subloop的个数
    void setThreadNum(int numThreads);

    //开启服务器监听
    void start();
private:
    void newConnection(int sockfd,const InetAddress & peerAddr);
    void removeConnection(const TcpConnectionPtr & conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std:: unordered_map<std::string, TcpConnectionPtr>;
    
    EventLoop * loop_; // baseloop 用户定义的loop

    const std::string ipPort_;//保存服务器ip 和 port
    const std::string name_; //保存服务器名称

    std::unique_ptr<Acceptor> acceptor_;// 运行在mainLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; //one Loop per thread

    ConnectionCallback connectionCallback_; //有连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调

    ThreadInitCalllback threadInitCalllback_;//loop线程初始化的回调

    std::atomic_int started_;

    int nextConnId_; //连接的id
    ConnectionMap connections_; // 保存所有的连接
};