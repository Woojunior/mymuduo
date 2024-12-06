#include"Poller.h"
#include"EPollPoller.h"

#include<stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL")){
        //如果设置环境变量为MUDUO_USE_POLL，则使用Poll
        return nullptr;//生成poll的实例
    }else{
        //默认为epoll
        return new EPollPoller(loop);//生成epoll的实例
    }
}