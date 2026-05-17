#include "ThreadPool.h"
#include "Logger.h"

ThreadPool::ThreadPool(const std::string &nameArg)
    : mutex_()
    , notEmpty_()
    , notFull_()
    , name_(nameArg)
    , maxQueueSize_(0)
    , running_(false)
{}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i + 1);
        threads_.emplace_back(new Thread(
            std::bind(&ThreadPool::runInThread, this), name_ + id));
        threads_[i]->start();
    }
    // 不创建新线程
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        notEmpty_.notify_all(); // 唤醒所有等待线程
        notFull_.notify_all();
    }
    // 等待所有子线程退出
    for (auto &thr : threads_)
    {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}

// 单线程直接运行task，多线程送入任务队列中
void ThreadPool::run(Task task)
{
    if (threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(isFull() && running_)
        {
            notFull_.wait(lock);
        }
        if (!running_) return;

        queue_.push_back(std::move(task));
        notEmpty_.notify_one(); // 新增一个任务，唤醒一个消费者线程
    }
}

// 任务队列是否满了
bool ThreadPool::isFull() const
{
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    try
    {
        if (threadInitCallback_)
        {
            threadInitCallback_();
        }
        while(running_)
        {
            Task task(take());
            if (task)
            {
                task();
            }
        }
    }
    catch(...)
    {
        LOG_ERROR("runInThread throw exception");
    }
}

// 从任务队列中取任务
ThreadPool::Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(mutex_);

    while(queue_.empty() && running_)
    {
        notEmpty_.wait(lock);
    }
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0)
        {
            notFull_.notify_one();
        }
    }
    return task;
}