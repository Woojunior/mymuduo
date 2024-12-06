#pragma once
#include<iostream>
#include<cstdint> //包含int64_t
#include<string>
//时间类
class Timestamp
{
public:
    Timestamp();//默认构造函数
    explicit Timestamp(int64_t microSecondSinceEpoch);//带参数的构造函数
    static Timestamp now();//获得当前的时间 长整型的形式保存在microSecondSinceEpoch中
    std::string toString() const; //将长整型时间，转换成字符串的形式 年月日时分秒

private:
    int64_t microSecondSinceEpoch_;
};