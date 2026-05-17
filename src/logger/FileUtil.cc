#include "FileUtil.h"
#include "Logging.h" // 使用 getErrnoMsg

FileUtil::FileUtil(const std::string &fileName)
    : fp_(::fopen(fileName.c_str(), "ae"))  // ‘a’是追加，'e'表示O_CLOEXEC
    , writtenBytes_(0)
{
    // 是在给文件流设置用户自定义缓冲区，这样调用write的时候才会借助缓冲区，并在适当的时机写入文件
    ::setbuffer(fp_, buffer_, sizeof buffer_);
}

FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

// 不是线程安全的
void FileUtil::append(const char *data, size_t len)
{
    // 记录已经写入的数据大小
    size_t written = 0;
    while(written != len)   // 只要没写完就一直写
    {
        size_t remain = len - written;  // 还需写入的数据大小
        size_t n = write(data + written, remain);   // 返回成功写入了n字节
        if (n != remain)    // 剩下的没有写完就看下是否出错，没出错就继续写
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
            }
        }
        written += n;   // 更新写入的数据大小
    }
    writtenBytes_ += written;   // 记录目前为止写入的数据大小，超过限制会滚动日志(滚动日志在LogFile中实现)
}

// 强制把 FILE* 用户态缓冲区的数据立刻刷新到内核, 不是直接刷到磁盘
void FileUtil::flush()
{
    ::fflush(fp_);
}

// 向 FILE* 的用户态缓冲区写数据，通常先写入 buffer_
// 不是线程安全的
size_t FileUtil::write(const char *data, size_t len)
{
    /**
     * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
     * -- buffer:指向数据块的指针
     * -- size:每个数据的大小，单位为Byte(例如：sizeof(int)就是4)
     * -- count:数据个数
     * -- stream:文件指针
     */
    // fwrite_unlocked不是线程安全的
    // data => buffer_ => 缓冲区满了 => 内核缓冲区 => 磁盘
    return ::fwrite_unlocked(data, 1, len, fp_);
}