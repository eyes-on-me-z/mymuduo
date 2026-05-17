#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

/**
 * 从fd上读取数据  Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 * struct iovec {
    void  *iov_base; // 缓冲区起始地址
    size_t iov_len;  // 长度
    };
 */ 
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间  64K分配快（无需 malloc）, 生命周期短（函数结束自动释放）

    struct iovec vec[2];

    const size_t writeable = wirteableBytes();  // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;   // buffer不够64kB，用两个缓冲区
    const ssize_t n = ::readv(fd, vec, iovcnt); // 一次 read → 全部读完 → 再决定是否扩容
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writeable)    // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else    // extrabuf里面也写入了数据 
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeable);    // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

// 这个函数从可读缓冲区读取数据，为什么不用改变readerIndex_
// 我感觉存在一点问题
// 他是在外面retrieve，。。。。
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }

    return n;
}