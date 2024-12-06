#pragma once
#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid(){
        if(__builtin_expect(t_cachedTid==0,0)){
            //(t_cachedTid==0,0) 第一个参数表示判断t_cachedTid是否为0，第二参数表述第一个参数所使用的条件很可能为假(t_cachedTid!=0)
            //条件为真时，进行系统调用 得到tid
            cacheTid();//通过第一次调用得到tid，将其存在t_cachedTid中
            //这样做的目的是提高程序的执行的效率，避免反复进行系统调用查看tid
        }
        //条件为假，则直接返回，代表t_cachedTid中已经缓存了tid
        return t_cachedTid;
    }
}