#include <sys/time.h>
#include <inttypes.h>

#include "Timestamp.h"

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{}

// 获取当前时间，并以微秒级时间戳形式返回一个 Timestamp 对象
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);    // 获取当前系统时间
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

// 秒.微秒 格式的时间字符串
std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    // PRId64 = 跨平台打印 64 位整数
    snprintf(buf, sizeof buf, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

// #include <iostream>

// int main()
// {
//     std::cout << Timestamp::now().toString() << std::endl;
// }