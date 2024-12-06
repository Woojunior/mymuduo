#include"Buffer.h"

#include<errno.h>
#include<sys/uio.h>
#include<unistd.h>
//从fd上读取数据
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536]={0}; // 栈上的内存空间 64K
    struct iovec vec[2]; //读取的内存块

    const size_t writable = writableBytes();//Buffer底层缓冲区剩余的 可写的大小
    //将剩余的的这部分 装在第一个块中
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    //额外开辟的 装在第二个块中
    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1 ; //选择都一块还是两块
    const ssize_t n = ::readv(fd,vec,iovcnt);

    if(n<0)
    {
        *saveErrno=errno;
    }
    else if (n<=writable)//Buffer的可写缓存区已经足够储存读出来的数据了
    {
        writerIndex_ += n ;
    }
    else //Buffer的可写缓存区不够
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writable); //将Buffer没存完的数据，储存在额外的缓存中
    }

    return n;
}
//向fd上写入数据
ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd,peek(),readableBytes());
    if( n<0 )
    {
        *saveErrno=errno;
    }
    return n;
}