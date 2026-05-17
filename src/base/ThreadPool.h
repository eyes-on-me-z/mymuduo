#pragma once

#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include <vector>
#include <deque>
#include <cstring>

#include "noncopyable.h"
#include "Thread.h"

class ThreadPool : noncopyable
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(const std::string &nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task &cb)
    { threadInitCallback_ = cb; }

    void start(int numThreads);
    void stop();

    const std::string& name() const
    { return name_; }

    size_t queueSize() const;

    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();

    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;

    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};