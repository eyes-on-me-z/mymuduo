#pragma once

#include <string>
#include <mutex>
#include <memory>

#include "noncopyable.h"
#include "FileUtil.h"

/**
 * LogFile类用于向文件中写入日志信息，具有在合适时机创建日志文件以及把数据写入文件的功能
*/
class LogFile : noncopyable
{
public:
    // threadSafe表示是否需要考虑线程安全，其的作用：同步日志需要锁，因为日志可能来自多个线程，异步日志不需要锁，
    // 因为异步日志的日志信息全部只来自异步线程（见AsyncLogging::threadFunc），无需考虑线程安全。
    // 由于默认是同步日志，因此threadSafe默认为true
    LogFile(const std::string basename,
            off_t rollSize,
            bool threadSafe = true,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char *logline, int len);

    // 根据当前时间生成一个文件名并返回
    static std::string getLogFileName(const std::string &basename, time_t *now);

    const std::string basename_;
    const off_t rollSize_;  // 是否滚动日志（创建新的文件存储日志）的阈值，file_的buffer_中的数据超过该值就会滚动日志
    const int flushInterval_;
    const int checkEveryN_; // 记录往buffer_添加数据的次数,超过checkEveryN_次就fflush一下

    int count_;

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;   // 一天的秒数
};