#pragma once

#include <atomic>

#include "noncopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"

/**
 * 定时器类：一个定时器应该需要知道超时时间，是否重复，如果是重复的定时器就需要知道执行间隔是多少，
 * 还需要知道超时后进行什么样的处理（执行回调函数）
*/

class Timer : noncopyable
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb))
        , expiration_(when)
        , interval_(interval)
        , repeat_(interval > 0.0)
        , sequence_(s_numCreated_.fetch_add(1) + 1)
    {}

    // 定时器到期后，调用回调函数
    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    // 重启定时器，其实就是修改一下下次超时的时刻
    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_.load(); }

private:
    const TimerCallback callback_;  // 定时器到期后要执行的回调函数
    Timestamp expiration_;  // 超时时刻
    const double interval_; // 超时时间间隔，如果是一次性定时器，则该值应该设为0
    const bool repeat_; // 是否可重复使用（false表示一次性定时器）
    const int64_t sequence_;

    static std::atomic<int64_t> s_numCreated_;
};