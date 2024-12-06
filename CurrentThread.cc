#include"CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid=0;//__thread是GCC和Clang扩展的关键字，每个线程都会拥有自己独立的这个变量的副本，而不是所有线程共享同一个变量

    void cacheTid()
    {
        if(t_cachedTid==0){

            //通过linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t> (::syscall(SYS_gettid));
        }
    }

}