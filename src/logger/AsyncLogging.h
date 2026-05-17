#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"
#include "FixedBuffer.h"
#include "Thread.h"

class AsyncLogging : noncopyable
{
public:
    //（basename时间.log）
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // 前端调用 append 写入日志
    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    void threadFunc();

    using Buffer = FixedBuffer<kLargeBuffer>;   // 4M
    // BufferVector存储独占指针的目的是：从BufferVector中弹出元素（Buffer）后会自动析构
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;   // 刷新时间间隔（s）
    std::atomic<bool> running_;
    const std::string basename_;    // 日志文件名的公共的部分
    const off_t rollSize_;          // 日志文件超过多大时创建新的日志文件
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    
    BufferPtr currentBuffer_;   // 前端日志就是写入这个缓冲区 4M
    BufferPtr nextBuffer_;      // 备用缓冲区
    BufferVector buffers_;      // 用于存储已经写满的BufferPtr，便于后台线程写入文件，每个元素对应的缓冲区都是4M
};