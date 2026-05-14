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
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    
    BufferPtr currentBuffer_;   // 前端日志就是写入这个缓冲区 4M
    BufferPtr nextBuffer_;      // 备用缓冲区
    BufferVector buffers_;      // 用于存储已经写满的BufferPtr，便于后台线程写入文件，每个元素对应的缓冲区都是4M
};