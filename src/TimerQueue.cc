#include "TimerQueue.h"
#include "Logger.h"
#include "TimerId.h"
#include "Timer.h"
#include "EventLoop.h"
#include "Timestamp.h"

#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <cstring>

static int createTimerfd()
{
    //  CLOCK_MONOTONIC: 单调递增、永不回退的系统时钟
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (timerfd < 0)
    {
        LOG_FATAL("failed in timerfd_create");
    }
    return timerfd;
}

// 返回当前时刻距 when 还有多少时间（< 100us 则设置为100us）
static struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                            - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(    // 多少秒
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(     // 多少纳秒
        (microseconds % Timestamp::kMicroSecondsPerSecond ) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_INFO("TimerQueue::handleRead() %llu at %s\n", howmany, now.toString().c_str());
    if (n != sizeof howmany)
    {
        LOG_ERROR("TimerQueue::handleRead() reads %d bytes instead of 8\n", n);
    }
}

// 重置 timerfd 定时器的到期时间
// 改变了timers_的最早到期时间，都需要调用
static void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue; // 要设置的新时间
    struct itimerspec oldValue; // 保存旧时间（用不到，但传进去）
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    // 计算：从现在到到期时间还剩多久
    newValue.it_value = howMuchTimeFromNow(expiration);

    // 真正设置定时器
    // 0 = 相对时间
    // &newValue = 新配置
    // &oldValue = 接收旧配置（这里只是接收，不用）
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_ERROR("timerfd_settime()\n");
    }
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(loop, timerfd_)
    , timers_()
    , callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this)
    );
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);

    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}

// 通过调用insert向TimerList中插入定时器（回调函数，到期时间，时间间隔）
TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer)
    );

    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(
        std::bind(&TimerQueue::cancelInLoop, this, timerId)
    );
}

// 在自己所属的loop中添加定时器
void TimerQueue::addTimerInLoop(Timer *timer)
{
    bool earliestChanged = insert(timer);

    // 如过插入定时器后，最找的超时时间改变了，需要重置定定时器
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 在自己所属的loop中取消定时器
void TimerQueue::cancelInLoop(TimerId timerId)
{
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        delete it->first;
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_) // 所需要取消的定时器正在被调用
    {
        cancelingTimers_.insert(timer);
    }
}

// 定时器读事件触发的函数(定时器到期)
void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    // 如果定时器正在被调用则 cancelingTimers_.insert(timer)
    // 有没有可能 刚执行上面的代码，立马就执行 cancelingTimers_.clear()呢？
    cancelingTimers_.clear();

    for (const Entry &it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

// 获取到期的定时器，将到期的定时器从timers_和activeTimers_中移除
// 所有超时时间在now之前的都是过期的
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired; // 存储所有到期的定时器
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));   // 指针能表示的最大数值（就是 0xffffffffffffffff）
    TimerList::iterator end = timers_.lower_bound(sentry);  // 返回第一个 >= sentry 的元素的迭代器
    std::copy(timers_.begin(), end, back_inserter(expired));    // back_inserter = 自动在尾部追加元素
    timers_.erase(timers_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        activeTimers_.erase(timer);
    }

    return expired;
}

// 重置这些到期的定时器（销毁或者重复定时任务）
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }

    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }

}

// 插入定时器的内部方法
bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    // 判断timers_最早的超时时刻是否被改变
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    timers_.insert(Entry(when, timer));
    activeTimers_.insert(ActiveTimer(timer, timer->sequence()));

    return earliestChanged;
}

/*
struct itimerspec {
    struct timespec it_interval;  // 间隔时间：定时器循环触发的周期 =0 表示只触发一次
    struct timespec it_value;     // 第一次触发定时器的时间 =0 表示不启动定时器
};

struct timespec {
    time_t tv_sec;   // 秒
    long tv_nsec;    // 纳秒
};
*/