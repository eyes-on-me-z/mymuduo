#pragma once

#include <string>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); //禁止“隐式类型转换”，只能显式调用构造函数
    static Timestamp now();
    static Timestamp invalid()
    {
        return Timestamp();
    }
    
    std::string toString() const;

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_;
};

// 如果是重复定时任务就会对此时间戳进行增加。
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    // 将延时的秒数转换为微秒
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    // 返回新增时后的时间戳
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

/* 
Timestamp t = 100;   // 隐式转换
*/