#include"EPollPoller.h"
#include"Logger.h"
#include"Channel.h"

#include<errno.h>
#include<unistd.h>
#include<strings.h>

//channel未添加到poller中
const int kNew = -1; //Channel的成员index=-1；
//channel已添加到poller中
const int kAdded = 1;
//channel 从Poller中删除
const int kDeleted=2;

//构造和析构
EPollPoller::EPollPoller(EventLoop * loop)
    :Poller(loop)//Poller基类的构造函数
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    ,events_(kInitEventListSize) //vector<epoll_event>

{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d \n" , errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}   

Timestamp EPollPoller::poll(int timeoutMS, ChannelList* activeChannels)
{
    //实际上应用LOG_DEBUG输出更为合理
    LOG_INFO("func=%s => fd total count:%lu \n" , __FUNCTION__ , channels_.size());

    //numEvents返回的是就绪文件的描述符的个数
    int numEvents = ::epoll_wait(epollfd_, &(*events_.begin()),static_cast<int> (events_.size()),timeoutMS);
    int saveError=errno;
    Timestamp now(Timestamp::now());//获得一个名为now的对象，其中保存的是当前的时间

    if(numEvents>0){
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents,activeChannels);//将活跃的事件填入 事件表中
        if(numEvents==events_.size()){
            //如果活跃事件的个数等于events_数组的大小，则要对数组进行扩容
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents==0)
    {
        //超时
        LOG_DEBUG("%s timeout! \n" , __FUNCTION__);

    }
    else{
        //numsEvents小于零，则错误
        if(saveError!=EINTR){
            errno=saveError;
            LOG_ERROR("EPollPoller::poll() error!");
        }
    }
    return now;//返回当前的时间

}


//调用关系：channel update/remove => EventLoop updateChannel/removeChannel => Poller/updateChannel removeChannel
/**
 *              EventLoop => poller.poll
 *      ChannelList             Poller
 *                       ChannelMap<fd,channel*> epollfd
 */
//更新Poller中新的channel状态
void EPollPoller::updateChannel(Channel* channel)
{
    const int index=channel->index();//获取管道中的状态，index
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n" , __FUNCTION__, channel->fd(),channel->events(),index);
    
    if(index==kNew || index==kDeleted)
    {
        if(index==kNew){
            int fd = channel->fd();
            channels_[fd]=channel;//Poller中的ChannelMap类，将sockfd和channel对应起来
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
        
    }
    else
    {   //index==kAdded
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }else{
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

//从Poller中删除channel
void EPollPoller::removeChannel(Channel* channel){
    int fd=channel->fd();
    channels_.erase(fd);//从ChannelMap中删除对应的channel

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__,fd);

    int index=channel->index();//等到当前channel的index状态
    if(index==kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    
    channel->set_index(kNew);

}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents , ChannelList * activeChannels) const
{
    for(int i = 0 ; i<numEvents ; i++){
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//EventLoop 就拿着它的poller给它返回的所有事件列表了
    }
}


//更新channel通道epoll_ctl add/mod/del
void EPollPoller::update(int operation , Channel* channel)
{
    epoll_event event;
    bzero(&event,sizeof(event));//清空内存

    int fd=channel->fd();

    event.events = channel->events();//得到是EPOLLIN/EPOLLOUT等表示
    event.data.fd=fd;//将channel相关的fd传入用户数据这个结构中
    event.data.ptr=channel;//将channel这个指针保存在用户数据这个结构中

    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        //设置失败
        
        if(operation==EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n" ,errno);
        }else{
            LOG_FATAL("epoll_ctl add/mod error:%d\n" , errno);
        }
    }
}