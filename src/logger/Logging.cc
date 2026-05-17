#include <ctime>

#include "Logging.h"
#include "CurrentThread.h"

namespace ThreadInfo
{
    __thread char t_errnobuf[512];  // 存储错误信息
    __thread char t_time[64];   // 存储格式化的时间字符串
    __thread time_t t_lastSecond;   // 存储最近一次调用的时间
};

const char* getErrnoMsg(int savedErrno)
{
    // strerror()线程不安全。它内部使用静态缓冲区
    // 把错误信息写入当前线程自己的 buffer
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof(ThreadInfo::t_errnobuf));
}

// 数组变量，返回日志等级对应的字符串
const char* LogLevelName[Logger::LogLevel::NUM_LOG_LEVELS] = 
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// 初始日志等级默认是 INFO
Logger::LogLevel initLogLevel()
{
    return Logger::LogLevel::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

// 把数据写入到stdout
static void defaultOutput(const char *msg, int len)
{
    // 向标准输出 stdout 写入 len 字节数据
    // 数据地址 每个元素大小 元素个数 输出目标
    fwrite(msg, 1, len, stdout);
}

// 刷新stdout
static void defaultFlush()
{
    fflush(stdout);
}

// 默认向终端输出
Logger::OutputFunc g_output = defaultOutput;
// 默认向终端刷新
Logger::FlushFunc g_flush = defaultFlush;

// level默认为INFO等级
Logger::Logger(const char *file, int line)
    : impl_(INFO, 0, file, line)
{}

Logger::Logger(const char *file, int line, Logger::LogLevel level)
    :impl_(level, 0, file, line)
{}

// 可以打印调用函数
Logger::Logger(const char *file, int line, Logger::LogLevel level, const char *func)
    :impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::~Logger()
{
    impl_.finish();
    // 获取buffer(stream_.buffer_)
    const LogStream::SmallBuffer& buf(stream().buffer());
    // 输出(默认向终端输出)
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        g_flush();  // 把缓冲区的数据强制写入指定的位置（默认是写入stdout）
        abort();
    }

}

void Logger::setLogLevel(LogLevel level)
{
    g_logLevel = level;
}

// 设置日志输出位置（由终端改为其他位置）
void Logger::setOutput(OutputFunc out)   
{
    g_output = out;
}

// 设置缓冲区刷新位置（由终端改为其他位置）
void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}


Logger::Impl::Impl(LogLevel level, int savedErrno, const char *file, int line)
    : time_(Timestamp::now())
    , stream_()
    , level_(level)
    , line_(line)
    , basename_(file)
{
    formatTime();
    stream_ << GeneralTemplate(LogLevelName[level], 6);
    if (savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// 把时间变成字符串的形式，并append到LogStream的buffer_中
void Logger::Impl::formatTime()
{
    // 我觉得原来的有点问题，所以这里参照muduo
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);

    struct tm *tm_time = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(ThreadInfo::t_time, sizeof(ThreadInfo::t_time), "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    // 更新最后一次时间调用
    ThreadInfo::t_lastSecond = seconds;

    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ".%06d ", microseconds);
    
    // 输出时间，附有微秒
    stream_ << GeneralTemplate(ThreadInfo::t_time, 19) << GeneralTemplate(buf, 8);
}

void Logger::Impl::finish()
{
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_) << ':' << line_ << '\n';
}

// 编译指令：g++ ./src/logger/Logging.cc ./src/logger/LogStream.cc ./src/Timestamp.cc -I ./src -o testLogging
#if 0
int main()
{
    LOG_INFO << "woxihuanni";
}
#endif