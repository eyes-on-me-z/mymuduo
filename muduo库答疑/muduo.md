#### 为什么不在EventLoop::handleRead()中执行doPendingFunctors()

1. handleRead()是从wakeupFd文件描述符里面读取数据，如果死绑在handleRead()，只有向wakeupFd里面写入数据，才会执行doPendingFunctors()，那么在IO线程内注册了回调函数并且没有调用EventLoop::wakeup()，那么回调函数不会被立即得到执行，而必须等待EventLoop::wakeup()被调用后才能被执行。
2. wakeupFd负责通知EventLoop有回调需要处理，handleRead()自身并不负责执行回调函数，
3. 一次wakeup()涉及三次系统调用（write，epoll_wait，read）
4. 如果在EventLoop所在线程调用queueInLoop()，且此时没有执行doPendingFunctors()函数，则不会发生wakeup()，需要执行的回调函数不能够及时执行，必须要wakeup()（三次系统调用）才能够执行回调函数。
5. 如果不是在 handleRead() 中调用 doPendingFunctors()，而是在 loop() 的 while 循环中直接调用，那么，这时候并不需要 wakeup() 就可以将这些新增的 cb() 执行完。

#### 怎么处理文件描述符耗尽情况？

​	fd是稀缺资源，如果连接达到上限，新来的连接accept失败，无法创建新的fd，则还处于全连接队列中，listenfd还是可读的，因为muduo采用的是LT模式，下一次epoll_wait的时候会立刻返回，程序陷入busy loop。

​	可以准备一个空闲的fd，遇到上述情况，先关闭空闲fd，再accept新的fd，随后立即关闭，再打开之前关闭的空闲的fd。（这种做法多线程情况下不安全，存在race condition）