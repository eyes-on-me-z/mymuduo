#include "FileUtil.h"
#include "Logging.h"

explicit FileUtil::FileUtil(const std::string &fileName)
    : fp_(::fopen(fileName.c_str(), "ae"))  // ‘a’是追加，'e'表示O_CLOEXEC
    , writtenBytes_(0)
{
    // 是在给文件流设置用户自定义缓冲区
    ::setbuffer(fp_, buffer_, sizeof buffer_);
}

FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

void FileUtil::append(const char *data, size_t len)
{
    size_t written = 0;
    while(written != len)
    {
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
            }
        }
        written += n;
    }
    writtenBytes_ += written;
}

// 强制把 FILE* 用户态缓冲区的数据立刻刷新到内核, 不是直接刷到磁盘
void FileUtil::flush()
{
    ::fflush(fp_);
}

// 向 FILE* 的用户态缓冲区写数据，通常先写入 buffer_
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