#include <cstring>
#include <arpa/inet.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    // std::string ip = inet_ntoa(addr_.sin_addr); //线程不安全, 内部用静态缓冲区，会被覆盖

    char buf[64] = {0};
    // 把 网络字节序的 32 位 IP → 点分字符串 IP
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));    // :: 表示全局作用域

    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);

    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>

// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;

//     return 0;
// }