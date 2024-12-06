#include"Channel.h"
#include"EventLoop.h"
#include"Logger.h"

#include<sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent= EPOLLOUT;

//定义构造函数和析构函数
Channel::Channel(EventLoop* loop , int fd) : loop_(loop),fd_(fd),events_(0),revents_(0),index_(-1),tied_(false)
{}
Channel::~Channel(){}

//Channel的tie方法什么时候调用过?一个TcpConnection新连接创建的时候 TcpConnection=>Channel
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_=obj;
    tied_=true;
}

/**
 * 当改变Channel所表示fd的events事件后，update负责在poller里面更改fd相应事件epoll_ctl
 * EventLoop-> ChannelList poller
 */
void Channel::update()
{
    //通过Channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    
    loop_->updateChannel(this);
}

//在Channel所属的EventLoop中，把当前的Channel删掉
void Channel::remove()
{
    //add code..
    loop_->removeChannel(this);
}

//fd得到Poller通知后，处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_){
        std::shared_ptr<void> guard=tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

//根据Poller通知的Channel发生的具体事件，由Channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime){
    LOG_INFO("Channel handleEvent revents:%d\n",revents_);

    //挂起
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_){
            closeCallback_();
        }
    }

    //错误
    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();
        }
    }

    //数据可读，高优先级数据可读
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }

    //数据可写(包括普通数据和优先数据)
    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}