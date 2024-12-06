#pragma once
#include<string>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>

//将ip和端口号绑定，表示TCP连接的一端
//储存在addr中的为网络字节序
class InetAddress
{
public:
    //显示定义带参数的构造函数 防止隐式转换
    explicit InetAddress(
        uint16_t port=0,//端口号
        std::string ip="127.0.0.1"//默认ip是127.0.0.1
     );//其中端口号和ip都是传到addr_中
    explicit InetAddress(const sockaddr_in& addr): addr_(addr) {};

    //获取ip和port的信息，这里涉及到网络字节序->本地字节序：小（大端就是网络字节序）端字节序
    std::string toIp() const; 
    std::string toIpPort() const;
    std::uint16_t toPort() const;

    //得到与socket绑定的地址
    const sockaddr_in* getSockAddr() const { return &addr_;}
    //设置于socket绑定的地址
    void setSockAddr(const sockaddr_in & addr) { addr_=addr; }
private:
    sockaddr_in addr_;
};