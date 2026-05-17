#include "Logger.h"

// 简易日志

// 获取日志唯一的实列对象
Logger &Logger::instance()
{
    /*
    静态对象，它的生命周期是“程序生命周期”，不会随调用处的作用域结束而析构。
    所以下次调用 Logger::instance() 仍然返回同一个对象，直到进程退出时才执行析构。
    */
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
// 写日志   [级别信息]时间 : 信息
void Logger::log(const std::string &msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}