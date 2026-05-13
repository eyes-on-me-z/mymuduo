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
private:
    void append_unlocked(const char *logline, int len);

    // 根据当前时间生成一个文件名并返回
    static std::string getLogFileName(const std::string &basename, time_t *now);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval;
    const int checkEveryN_;

    int count_;

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;   // 一天的秒数
};