# mymuduo

`mymuduo` 是一个参考 muduo 思路实现的 C++ 网络库，当前代码聚焦于 Reactor 网络编程模型的核心能力，并去除了对 Boost 的依赖。项目使用 CMake 构建，默认生成动态库 `libmymuduo.so`。

## 特性

- 基于 `epoll` 的事件分发：`EventLoop`、`Channel`、`Poller`、`EPollPoller`
- one loop per thread 模型：`EventLoopThread`、`EventLoopThreadPool`
- TCP 服务端封装：`TcpServer`、`Acceptor`、`TcpConnection`、`Socket`、`InetAddress`
- 应用层缓冲区：`Buffer`
- 定时器支持：`Timer`、`TimerId`、`TimerQueue`
- 日志模块：`Logger`、`LogStream`、`LogFile`、`AsyncLogging`
- 基础工具：`Thread`、`ThreadPool`、`Timestamp`、`noncopyable`
- 工具组件：`CircularBuffer`

## 目录结构

```text
.
├── CMakeLists.txt
├── autobuild.sh
├── example
│   ├── makefile
│   └── testserver.cc
├── src
│   ├── base       # 线程、线程池、时间戳、不可拷贝基类
│   ├── logger     # 同步/异步日志、日志文件、日志流
│   ├── net        # Reactor、TCP 服务端、连接、Socket、定时器、epoll 封装
│   └── utils      # 通用工具组件
└── muduo库答疑
```

## 核心模块

### base

基础模块提供网络库运行所需的通用能力：

- `Thread`：对 `std::thread` 的封装，记录线程名、启动状态和线程 id
- `ThreadPool`：任务队列和工作线程池
- `CurrentThread`：获取当前线程 id
- `Timestamp`：微秒级时间戳及时间计算
- `noncopyable`：禁止拷贝的基类

### logger

日志模块默认输出到标准输出，也支持通过 `Logger::setOutput` 和 `Logger::setFlush` 自定义输出位置。

常用日志宏：

```cpp
LOG_TRACE << "trace log";
LOG_DEBUG << "debug log";
LOG_INFO << "info log";
LOG_WARN << "warn log";
LOG_ERROR << "error log";
LOG_FATAL << "fatal log";
```

异步日志由 `AsyncLogging` 提供，内部使用前端缓冲区和后台线程批量落盘。

### net

网络模块是项目主体：

- `EventLoop`：事件循环，负责 `poll`、跨线程任务投递、定时器和唤醒机制
- `Channel`：封装文件描述符及其关注事件，绑定读、写、关闭、错误回调
- `Poller` / `EPollPoller`：IO 复用抽象及 Linux epoll 实现
- `Acceptor`：监听连接并接收新客户端
- `TcpConnection`：管理单条 TCP 连接的生命周期、收发缓冲区和回调
- `TcpServer`：面向用户的 TCP 服务端类，管理监听、连接集合和 sub loop 分发
- `TimerQueue`：基于 `timerfd` 的定时任务队列

### utils

`CircularBuffer` 提供环形缓冲区工具。

## 构建

项目使用 CMake，源码当前按 C++17 编译：

```bash
mkdir -p build
cd build
cmake ..
make
```

构建完成后，动态库会输出到项目根目录的 `lib` 目录：

```text
lib/libmymuduo.so
```

也可以直接执行项目提供的脚本：

```bash
./autobuild.sh
```

脚本会重新生成 `build` 目录内容，编译动态库，并将头文件和动态库安装到：

```text
/usr/local/include/mymuduo
/usr/local/lib/libmymuduo.so
```

如果当前用户没有 `/usr/local` 写权限，需要使用 `sudo ./autobuild.sh`。

## 使用示例

下面是一个简单的 Echo Server，用法与 `example/testserver.cc` 一致：

```cpp
#include <mymuduo/TcpServer.h>
#include <mymuduo/Logging.h>

#include <functional>
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : loop_(loop)
        , server_(loop, addr, name)
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));

        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connection UP: " << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "Connection DOWN: " << conn->peerAddress().toIpPort();
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr, "EchoServer");

    server.start();
    loop.loop();

    return 0;
}
```

编译示例：

```bash
cd example
make
./testserver
```

测试连接：

```bash
telnet 127.0.0.1 8080
```

## 常用接口

### TcpServer

```cpp
TcpServer(EventLoop *loop,
          const InetAddress &listenAddr,
          const std::string &nameArg,
          TcpServer::Option option = TcpServer::kNotReusePort);

void setThreadNum(int numThreads);
void setConnectionCallback(const ConnectionCallback &cb);
void setMessageCallback(const MessageCallback &cb);
void setWriteCompleteCallback(const WriteCompleteCallback &cb);
void start();
```

### TcpConnection

```cpp
bool connected() const;
void send(const std::string &buf);
void send(Buffer *buf);
void shutdown();
void setTcpNoDelay(bool on);
void setContext(const std::any &context);
const std::any& getContext() const;
```

### EventLoop 定时器

```cpp
TimerId runAt(Timestamp time, TimerCallback cb);
TimerId runAfter(double delay, TimerCallback cb);
TimerId runEvery(double interval, TimerCallback cb);
void cancel(TimerId timerId);
```

## 注意事项

- 当前实现面向 Linux，依赖 `epoll`、`eventfd`、`timerfd` 等 Linux 系统接口。
- `autobuild.sh` 会写入 `/usr/local/include/mymuduo` 和 `/usr/local/lib`。
- 头文件安装脚本会把所有 `.h` 文件平铺复制到 `/usr/local/include/mymuduo`，因此示例使用 `<mymuduo/TcpServer.h>` 这类包含方式。
- 日志头文件当前为 `Logging.h`，使用日志宏时请包含 `<mymuduo/Logging.h>`。
