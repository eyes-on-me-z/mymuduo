#include <cerrno>
#include <unistd.h>
#include <strings.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

// channel未添加到poller中
const int kNew = -1;    // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))   //调用 exec 时自动关闭这个 epollfd_
    , events_(kInitEventListSize)   // vector<epoll_event>
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count: %lu\n", __FUNCTION__, channels_.size());

    int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (events_.size() == numEvents)
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout\n", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }

    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *            EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 */ 
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d, events=%d, index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            channels_[channel->fd()] = channel;     // 为什么kDeleted就不需要向channels_中添加键值对
        }
        update(EPOLL_CTL_ADD, channel);
        channel->set_index(kAdded);
    }
    else    // channel已经在poller上注册过了
    {
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);   // 设置成kDeleted并不会从channels_中删除，只是从epoll关心的fd中删除
        }                                   // channels_中的fd未必都会注册进epoll模型
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 调用updateChannel去删除时，只会从poller模型所关心的fd中删除，并不会从channels中移除
// 调用removeChannel时，既会从poller关心的fd中移除，也会从channels中移除

// 从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    channels_.erase(channel->fd());

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, channel->fd());

    if (channel->index() == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道 epoll_ctl add/mod/del
// 所有的向epoll模型add/mod/del都必须调用这个函数
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        else
            LOG_FATAL("epoll_ctl add or del error: %d\n", errno);
    }
}