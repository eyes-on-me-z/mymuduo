#include <unistd.h>
#include <strings.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "Logger.h"

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (struct sockaddr*)localaddr.getSockAddr(), sizeof (sockaddr_in)))
    {
        LOG_FATAL("bind sockfd: %d fail\n", sockfd_);
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd: %d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     */ 
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    bzero(&peer, sizeof(peer));
    int connfd = ::accept4(sockfd_, (struct sockaddr*)&peer, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(peer);
    }
    
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

/*
Nagle 算法默认是开启的，作用是：
攒够一定量数据再发送
减少小包、降低网络开销
*/
void Socket::setTcpNoDelay(bool on)
{
    // 1：开启 NO_DELAY；0：关闭
    int optval = on ? 1 : 0;
    // 设置 TCP_NODELAY 选项
    ::setsockopt(
        sockfd_,    // 当前 socket 文件描述符
        IPPROTO_TCP,    // 协议层：TCP 层
        TCP_NODELAY,    // 选项名：关闭/开启 Nagle 算法，TCP_NODELAY 只影响发送，不影响接收
        &optval,    // 选项值
        sizeof optval); // 选项长度
}

// 可以立刻重启服务器，不会被 TIME_WAIT 占用端口导致启动失败
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

// 允许多个进程 / 线程绑定同一个端口
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

// 自动检测 TCP 连接是否还活着，防止死连接、半开连接占用资源。
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}