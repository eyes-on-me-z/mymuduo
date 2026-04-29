### 把==定时器==、==异步双缓冲日志==系统、数据库、内存池、http业务等都加进来才是完全体

### （待完成）

#### 为什么不在EventLoop::handleRead()中执行doPendingFunctors()

1. handleRead()是从wakeupFd文件描述符里面读取数据，如果死绑在handleRead()，只有向wakeupFd里面写入数据，才会执行doPendingFunctors()，那么在IO线程内注册了回调函数并且没有调用EventLoop::wakeup()，那么回调函数不会被立即得到执行，而必须等待EventLoop::wakeup()被调用后才能被执行。
2. wakeupFd负责通知EventLoop有回调需要处理，handleRead()自身并不负责执行回调函数，
3. 一次wakeup()涉及三次系统调用（write，epoll_wait，read）
4. 如果在EventLoop所在线程调用queueInLoop()，且此时没有执行doPendingFunctors()函数，则不会发生wakeup()，需要执行的回调函数不能够及时执行，必须要wakeup()（三次系统调用）才能够执行回调函数。
5. 如果不是在 handleRead() 中调用 doPendingFunctors()，而是在 loop() 的 while 循环中直接调用，那么，这时候并不需要 wakeup() 就可以将这些新增的 cb() 执行完。

#### 怎么处理文件描述符耗尽情况？

​	fd是稀缺资源，如果连接达到上限，新来的连接accept失败，无法创建新的fd，则还处于全连接队列中，listenfd还是可读的，因为muduo采用的是LT模式，下一次epoll_wait的时候会立刻返回，程序陷入busy loop。

​	可以准备一个空闲的fd，遇到上述情况，先关闭空闲fd，再accept新的fd，随后立即关闭，再打开之前关闭的空闲的fd。（这种做法多线程情况下不安全，存在race condition）

#### 连接建立过程

在mainloop当中，先构造TcpServer对象，然后在TcpServer内部初始化acceptor的各个组件，之后accpetor生成listenfd 把他封装到acceptchannel当中，再调用acceptchannel.enablereading让poller监听有没有读事件（如果有就是有新连接），如果检测到，会通知accept处理，accept接到通知后调用handleRead方法，继续往后说就是主线程执行回调 newConnection 接收一个connfd后，选择一个ioloop，初始化TcpConnection进行判断 如果为当前线程那么就直接 runningloop 反之 queueinloop 唤醒该线程，之后subloop 注册事件，触发连接建立回调，连接就绪等待客户端数据 

#### newDefaultPoller为什么要单独在DefaultPoller.cc中实现

 因为newDefaultPoller是一个static静态方法，在外部使用的时候，需要通过Poller::newDefaultPoller()返回一个对象指针，依靠类名访问，这样对用户使用而言，只暴露了一个Poller类，像EPollPoller类和PollPoller类我们可以压根就不知道。如果设计成virtual虚函数，那么这个函数是依赖对象调用的，不是依靠类来调用了，这样对象都创建好了，还要newDefaultPoller创建对象干什么。那么结果就有两个，要么没有这个方法，要么就是静态方法。如果是静态方法，那么在对象的源文件就实现，就会陷入递归包含头文件的情况。像工厂模式、单例模式这些设计模式很多都用static通过类名加::域访问符来访问这些方法获取对象，典型的，智能指针，我们明明可以直接利用构造函数创建，为什么还要有std::make_shared<>()呢，这就是人家实现的创建对象的方式更加的安全且高效，而照样暴露普通的构造是给我们更多的灵活性，操作空间。

#### 关于CurrentThread.cc

为啥要使用系统调用获取tid：pthread使用的1:1的线程模型，底层的内核级线程是LWP（Light Weight Process，轻量级进程)。pthread_self返回的线程ID是用户级的线程ID，POSIX标准没有定义返回内核级的线程ID的函数，因为pthread是跨平台的，POSIX标准不保证每个平台有内核线程概念。

`pid_t` 是一个类型（typedef），表示**进程 ID**。`tid`（thread id）表示**线程 ID**

#### TcpConnection使用shared_ptr来管理的原因

TcpConnection是短命的，生命周期不一定由我们控制，某个时刻客户端断开连接，我们并不能立马delete这个对象。其他地方也可能持有它的引用。当旧的TCP发来一个request，处理这个request需要一点时间，在此期间如果旧的TCP连接一断开，立马销毁对象，关闭旧的文件描述符，而新的连接的sockfd可能和旧的fd一样，造成原本往旧的tcp连接发送的数据发给了新的tcp连接。为了应对这种情况，使用shared_ptr来管理TcpConnection的生命周期。

#### muduo主动关闭连接和被动关闭连接行为

主动关闭连接：TcpConnection::shutdown() => TcpConnection::shutdownInLoop() => Socket::shutdownWrite（关闭本地写端，等对方关闭后，再关闭本地读端，确保不会漏收数据）

被动关闭：TcpConnection::handleClose() => connectionCallback() 和 closeCallback() => TcpServer::removeConnection => TcpServer::removeConnectionInLoop => TcpConnection::connectionDestoryed => channel_->remove()







tcpConnection的三个半事件以及生命周期问题、one Loop per thread的实现方式才是我对muduo感到最震撼的地方