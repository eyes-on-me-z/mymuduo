#include <algorithm>
#include <cstdint>

#include "LogStream.h"

// 用于把数字转化位对应的字符
static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                                '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static const char digitsHex[] = "0123456789ABCDEF";

// 将指针格式化到buf中，返回所占字符个数(0x1234 => '1234')
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

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}

LogStream& LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(char v)
{
    buffer_.append(&v, 1);
    return *this;
}

LogStream& LogStream::operator<<(const void *p)
{
    // 把指针 p 转换成一个整数类型 uintptr_t
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }

    return *this;
}

LogStream& LogStream::operator<<(const char *str)
{
    if (str)
    {
        buffer_.append(str, strlen(str));
    }
    else
    {
        buffer_.append("(null)", 6);
    }

    return *this;
}

LogStream& LogStream::operator<<(const unsigned char *str)
{
    // char: -128 ~ 127  unsigned char: 0 ~ 255
    // reinterpret_cast: 按“底层二进制位”重新解释数据类型。
    return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string &str)
{
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream& LogStream::operator<<(const SmallBuffer &buf)
{
    *this << buf.toString();
    return *this;
}

LogStream& LogStream::operator<<(const GeneralTemplate &g)
{
    buffer_.append(g.data_, g.len_);
    return *this;
}

// 将整数处理成字符串并添加到buffer_中
template<class T>
void LogStream::formatInteger(T num)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *start = buffer_.current();
        char *cur = start;;
        const char *zero = digits + 9;
        bool negative = (num < 0);

        do
        {
            int remainder = static_cast<int>(num % 10);
            *(cur++) = zero[remainder];
            num /= 10;
        } while (num != 0);
        
        if (negative)
        {
            *(cur++) = '-';
        }
        *cur = '\0';

        std::reverse(start, cur);
        buffer_.add(static_cast<int>(cur - start));
    }
}