#include <algorithm>
#include <cstdint>

#include "LogStream.h"

// 用于把数字转化位对应的字符
static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                                '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static const char digitsHex[] = "0123456789ABCDEF";

// 将指针格式化到buf中，返回所占字符个数
size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;    // 一个无符号整数类型
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);   // 低位——高位 => 高位——低位

    return p - buf;
}

LogStream& LogStream::operator<<(bool v)
{
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int)
{
    
}
LogStream& LogStream::operator<<(unsigned int);
LogStream& LogStream::operator<<(long);
LogStream& LogStream::operator<<(unsigned long);
LogStream& LogStream::operator<<(long long);
LogStream& LogStream::operator<<(unsigned long long);

LogStream& LogStream::operator<<(float v);
LogStream& LogStream::operator<<(double v);
LogStream& LogStream::operator<<(char);

LogStream& LogStream::operator<<(const void *p);
LogStream& LogStream::operator<<(const char *str);
LogStream& LogStream::operator<<(const unsigned char *str);
LogStream& LogStream::operator<<(const std::string &str);
LogStream& LogStream::operator<<(const SmallBuffer &buf);
LogStream& LogStream::operator<<(const GeneralTemplate &g);

// 将整数处理成字符串并添加到buffer_中
template<class T>
void LogStream::formatInteger(T num)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *start = buffer_.current();
        char *cur = start;;
        const char *zero = digits + 9;
    }
}