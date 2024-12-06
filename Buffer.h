#pragma once

#include<vector>
#include<string>
#include<algorithm>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

//网络库底层缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // 前置区大小
    static const size_t kInitialSize = 1024; // 读写区大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize) //初始化缓冲区的大小
        , readerIndex_(kCheapPrepend) //初始化可读区的头部索引
        , writerIndex_(kCheapPrepend) //初始化可写区的头部索引  //缓冲区没有数据，最开始两个索引相同
    {}

    //可读区的大小
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    //可写区大小
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    //前置区的大小
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    //应用读取了数据，此时readerIndex所在的位置 onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;//应用只读取了缓冲区数据的一部分，就是len ，还剩下readerIndex+len这个位置到 writerIndex这个位置没读
        }else{
            retrieveAll();
        }
    }

    //应用将缓冲区中的数据读完了
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    //把onMessage函数上报的Buffer数据，转换成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); //将所有可读数据 转换成string类型
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);//将转成string的数据放在result中
        retrieve(len); //将读取出来的数据进行复位，即移动readerIndex
        return result;
    }

    //确保缓冲区有足够大的空间可以装下写入的数据
    void ensureWriteableBytes(size_t len)
    {
        if(writableBytes() < len){
            //装不下，扩容
            makeSpace(len);
        }
    }

    //把[data,data+len]内存上的数据，写入到可写缓冲区中
    void append(const char* data , size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    //得到可写缓冲区的头指针
    char* beginWrite()
    {
        return begin()+writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin()+writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    //向fd上写入数据
    ssize_t writeFd(int fd, int* saveErrno);
private:
    //返回vector底层 首元素的地址
    char* begin()
    {
        return &*buffer_.begin();
    }

    //方便常量元素调用
    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        /**
         * | prependable bytes |  readable bytes  |  writable bytes  |
         * | prependable bytes |                len                     |
         */
        //如果当前可写的长度+已经读取的长度，还是装不下len 则进行扩容
        if(writableBytes()+prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }else
        {   //能装下，但是要对应用程序没读完的可读部分进行移位, 移动到最开始的位置
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_ , begin() + writerIndex_ , begin() + kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;

        }
    }
    std::vector<char> buffer_;//定义缓冲区的容器
    size_t readerIndex_;//指向可读的开头
    size_t writerIndex_;//指向可写的开头
};