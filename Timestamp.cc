#include "Timestamp.h"
#include<time.h>

//默认构造函数
Timestamp::Timestamp()
{}

//带参数的构造函数
Timestamp::Timestamp(int64_t microSecondSinceEpoch):microSecondSinceEpoch_(microSecondSinceEpoch)
{}

//获得当前的时间 长整型的形式保存在microSecondSinceEpoch中
Timestamp Timestamp::now()
{
    return Timestamp(time(nullptr));
}


//将长整型时间，转换成字符串的形式 年月日时分秒
std::string Timestamp::toString() const
{
    char buf[128];
    tm *tm_time =localtime(&microSecondSinceEpoch_);
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year+1900,
        tm_time->tm_mon+1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);

    return buf;
}


// #include<iostream>
// int main(){
//     std::cout<<Timestamp::now().toString()<<std::endl;
//     return 0;
// }