#pragma once //防止头文件被重复包含

//定义这个基类的目的是
//方便其他类如果想要 不进行 拷贝和赋值 构造 直接继承这个类
//因为派生类的拷贝和赋值构造是先拷贝和赋值基类再进行派生类的拷贝和赋值


class nocopyable
{
public:
    nocopyable(const nocopyable&) = delete;//该类不进行拷贝构造
    nocopyable& operator=(const nocopyable&) =delete;//该类不进行赋值构造

protected://保护表示外部不能访问，但是继承类可以访问
    nocopyable()=default;
    ~nocopyable()=default;
};


