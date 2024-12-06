#include"InetAddress.h"
#include<string.h>

InetAddress::InetAddress(uint16_t port , std::string ip)
{
    bzero(&addr_, sizeof(addr_));//清空addr_内存
    addr_.sin_family=AF_INET;//IPv4
    addr_.sin_port=htons(port);//将端口号从本地字节序转换成网络字节序，再储存在addr中
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()) ;//首先将ip转成c风格的字符串，然后将点分十进制ip地址转换为二进制的网络字节序
}


//获取ip和port的信息，这里涉及到网络字节序->本地字节序：小（大端就是网络字节序）端字节序
std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));//将addr_.sin_addr中存的ip地址从网络字节序转换为本地字节序 存在buf字符串中
    return buf;
}   
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    size_t end=strlen(buf);//buf的长度
    uint16_t port=ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u" , port);//向已经写入ip的buf中的尾部写入port
    return buf;
}
std::uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}

//测试一下代码
// #include<iostream>
// int main () {
//     InetAddress addr(8080);//端口号8080，默认ip
//     std::cout<< addr.toIpPort()<< std::endl;
//     return 0;


// }