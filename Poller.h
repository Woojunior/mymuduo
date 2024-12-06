#pragma once

#include"nocopyable.h"
#include"Timestamp.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;
class Poller :nocopyable
{
public:
    using ChannelList=std::vector<Channel*>;

    //构造和析构
    Poller(EventLoop* loop);
    virtual ~Poller()=default;

    //给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMS, ChannelList* activeChannels)=0;//相当于epoll_wait,即在一段时间内等待文件描述符上的事件
    virtual void updateChannel(Channel* channel)=0;//相当于注册fd上的事件，epoll_ctl
    virtual void removeChannel(Channel* channel)=0;

    //判断参数channel是否在当前Poller当中
    bool hasChannel(Channel* channel) const;

    //Evetloop 可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    //map的key：sockfd   value：sockfd所属的channel通道类型
    using ChannelMap = std:: unordered_map<int , Channel*>;
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;//定义Poller所属的事件循环EventLoop

};