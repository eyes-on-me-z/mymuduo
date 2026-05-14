#include "LogFile.h"

LogFile::LogFile(const std::string basename,
            off_t rollSize,
            bool threadSafe,
            int flushInterval,
            int checkEveryN)
    : basename_(basename)
    , rollSize_(rollSize)
    , flushInterval_(flushInterval)
    , checkEveryN_(checkEveryN)
    , count_(0)
    , mutex_(threadSafe ? new std::mutex : nullptr) // 同步日志需要锁，而异步日志不需要锁
    , startOfPeriod_(0)
    , lastRoll_(0)
    , lastFlush_(0)
{
    rollFile();  // 滚动日志，相当于创建一个新的文件，用来存储日志
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

// 滚动日志（创建一个新的文件）
// basename + time + ".log"
// 会发生混动日志的情况：
// 1）日志大小超过了设定的阈值rollSize_
// 2）过了0点（注意不是每天0点准时创建，而是在append的时候判断刚才正在使用的日志文件是否是昨天/更久之前创建的，
// 只要今天还没有创建日志文件，那么就会在append的时候创建一个）
bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    // 计算现在是第几天 now/kRollPerSeconds求出现在是第几天，再乘以秒数相当于是当前天数0点对应的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start; // 更新一下startOfPeriod_，表示今天已经创建日志了
        file_.reset(new FileUtil(filename));    // 让file_指向一个名为filename的文件，相当于新建了一个文件
        return true;
    }

    return false;
}

void LogFile::append_unlocked(const char *logline, int len)
{
    file_->append(logline, len);
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else
    {
        ++count_;
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod > startOfPeriod_)
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

// 根据当前时间生成一个文件名并返回
std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;
    filename += ".log";

    return filename;
}