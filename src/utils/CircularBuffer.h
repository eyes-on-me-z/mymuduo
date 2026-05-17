#pragma once

// 待完成

#include <vector>

template<class T>
class CircularBuffer
{
public:
    class iterator
    {
    public:
        using
    private:
        CircularBuffer *buffer_;
        size_t pos_;
    };

private:
    size_t next(size_t index)
    { return (index + 1) % capacity_; }

    size_t prev(size_t index)
    { return (index - 1 + capacity_) % capacity_; }

    std::vector<T> buffer_;
    size_t capacity_;
    size_t size_;
    size_t head_;
    size_t tail_;
};