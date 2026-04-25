#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // __thread: 每个线程都有自己独立的一份变量
    extern __thread int t_cachedTid;     // 线程局部存储的全局变量，用来缓存当前线程的 ID。
                                         // extern 这个变量在别的 .cpp 文件里定义了，这里只是引用它。
    void cacheTid();

    inline int tid()    // 用来获取当前线程 ID
    {
        // 这是 GCC 的分支预测优化, 等价于：“编译器，我预计这个条件是 false（0）”
        // if (__builtin_expect(条件, 期望值))
        // 告诉 CPU：这个分支大概率不会发生
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}