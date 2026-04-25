#pragma once

#include <string>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); //禁止“隐式类型转换”，只能显式调用构造函数
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};

/* 
Timestamp t = 100;   // 隐式转换
*/