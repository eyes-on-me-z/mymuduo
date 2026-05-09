#pragma once

#include "FixedBuffer.h"

/**
 * GeneralTemplate:用于LogStream<<时间，sourceFIle等
*/
class GeneralTemplate : noncopyable
{
public:
    GeneralTemplate() : data_(nullptr), len_(0) {}

    explicit GeneralTemplate(const char* data, int len)
        : data_(data)
        , len_(len)
    {}

    const char *data_;
    int len_;
};

/**
 * LogStream： 实现类似cout的效果，便于输出日志信息，即：LogStream << A << B << ...
 * 要实现类似cout的效果，就需要重载“<<”
*/
class LogStream : noncopyable
{
public:
    using SmallBuffer = FixedBuffer<kSmallBuffer>;  // 4kB

    void append(const char *data, int len)
    { buffer_.append(data, len); }

    const SmallBuffer& buffer() const
    { return buffer_; }

    void resetBuffer()
    { buffer_.reset(); }

    LogStream& operator<<(bool v);

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float v);
    LogStream& operator<<(double v);
    LogStream& operator<<(char);

    LogStream& operator<<(const void *p);
    LogStream& operator<<(const char *str);
    LogStream& operator<<(const unsigned char *str);
    LogStream& operator<<(const std::string &str);
    LogStream& operator<<(const SmallBuffer &buf);
    LogStream& operator<<(const GeneralTemplate &g);


private:
    static const int kMaxNumericSize = 48;  // 数字（整数或者浮点数）转换为字符串后最大的长度
    SmallBuffer buffer_;

    template<class T>
    void formatInteger(T);
};