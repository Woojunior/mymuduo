#include"Poller.h"
#include"Channel.h"

Poller::Poller(EventLoop* loop): ownerLoop_(loop)
{

}

//判断参数channel是否在当前Poller当中
bool Poller::hasChannel (Channel* channel) const
{   //map的key：sockfd   value：sockfd所属的channel通道类型
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}