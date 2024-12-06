#pragma once
#include "nocopyable.h"

#include<string>

//LOG_INFO("%s %d" , arg1 ,arg2)
#define LOG_INFO(logmsgFormat,...)\
    do\
    { \
        Logger& logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0}; \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG //因为调试信息比较多，所以 如果定义了MUDEBUG 则打印调试信息，如果没有则不打印
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

//定义日志的级别
enum LogLevel{
    INFO,   //普通信息
    ERROR,  //错误信息(这里程序不一定终止)
    FATAL,  //core信息(这里也是错误，程序终止)
    DEBUG,  //调试信息
};

//输出一个日志类
class Logger : nocopyable
{
public:
    //获取日志唯一的实例对象
    static Logger& instance();

    //设置日志级别
    void setLogLevel(int level);

    //写日志
    void log(std::string msg);

private:
    int logLevel_;
    Logger(){}
};

