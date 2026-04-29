#pragma once

#include <set>
#include <atomic>
#include <vector>

#include "noncopyable.h"
#include "Channel.h"
#include "Callbacks.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    // 通过调用insert向TimerList中插入定时器（回调函数，到期时间，时间间隔）
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;  // 最早到期的定时器在 最前面（begin）
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    // 在自己所属的loop中添加定时器
    void addTimerInLoop(Timer *timer);

    // 在自己所属的loop中取消定时器
    void cancelInLoop(TimerId timerId);

    // 定时器读事件触发的函数
    void handleRead();

    // 获取到期的定时器
    std::vector<Entry> getExpired(Timestamp now);

    // 重置这些到期的定时器（销毁或者重复定时任务）
    void reset(const std::vector<Entry> &expired, Timestamp now);

    // 插入定时器的内部方法
    bool insert(Timer *timer);

    EventLoop *loop_;           // 所属的EventLoop
    const int timerfd_;         // timerfd是Linux提供的定时器接口
    Channel timerfdChannel_;    // 封装timerfd_文件描述符
    TimerList timers_;          // 定时器队列（内部实现是红黑树）

    ActiveTimerSet activeTimers_;
    std::atomic<bool> callingExpiredTimers_;    // 是否正在获取超时定时器
    ActiveTimerSet cancelingTimers_;
};