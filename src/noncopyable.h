#pragma once

// noncopyable被继承后，派生类对象可以正常构造和析构，
// 但派生类对象无法进行 拷贝构造 和 赋值操作

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};