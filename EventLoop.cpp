#include "EventLoop.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "EpollDispatcher.h"
#include "PollDispatcher.h"
#include "SelectDispatcher.h"
#include "Log.h"

EventLoop::EventLoop() : EventLoop(string())
{
}

EventLoop::EventLoop(const string threadName)
{
    m_isQuit = true; // 默认停止

    //m_dispatcher = new EpollDispatcher(this);
    //m_dispatcher = new PollDispatcher(this);
    m_dispatcher = new SelectDispatcher(this);

    m_tid = this_thread::get_id();
    m_tname = threadName == string() ? "Main" : threadName;

    // 初始化m_sv
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, m_sv);
    if (ret == -1)
    {
        perror("socketpair error");
        exit(1);
    }
#if 0
    //制定规则：sv[0] 发送数据  sv[1] 读数据   
    Channel* channel = new Channel(m_sv[1],FDEvent::readEvents,readLocalMessage,nullptr,nullptr,this);
#else
    auto obj = bind(&EventLoop::readMessage, this);
    // 制定规则：sv[0] 发送数据  sv[1] 读数据
    Channel *channel = new Channel(m_sv[1], FDEvent::readEvents, obj, nullptr, nullptr, this);
#endif

    // 将sv[1]添加到队列
    addTask(channel, Elemtype::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_isQuit = false;
    if (m_tid != this_thread::get_id())
    {
        return -1;
    }
    // 启动
    Debug("服务器启动了。。。");
    while (m_isQuit == false)
    {
        m_dispatcher->dispatch(2);
        processTask();
    }
    return 0;
}

int EventLoop::eventActivate(int fd, int event)
{
    if (fd < 0)
    {
        return -1;
    }
    Channel *channel = m_channelmap[fd];
    assert(channel->getfd() == fd);
    if (event & (int)FDEvent::readEvents && channel->read_cb)
    {
        channel->read_cb(const_cast<void *>(channel->getarg()));
    }
    if (event & (int)FDEvent::writeEvents && channel->write_cb)
    {
        channel->write_cb(const_cast<void *>(channel->getarg()));
    }
    return 0;
}

int EventLoop::addTask(Channel *channel, Elemtype type)
{
    // 初始化一个任务队列节点
    ChannelElement *cement = new ChannelElement;
    cement->channel = channel;
    cement->type = type;

    // 加锁
    m_mutex.lock();

    // 添加节点
    m_taskQ.push(cement);

    // 解锁
    m_mutex.unlock();

    // 处理节点      主线程监听连接，子线程处理节点
    if (this_thread::get_id() == m_tid) // （若这是子线程反应堆，只能子线程处理任务，主线程要去唤醒子线程）
    {
        processTask();
    }
    else // 主线程   调用子线程e反应堆的taskWakeup函数唤醒阻塞的子线程
    {
        taskWakeup();
    }
    return 0;
}

int EventLoop::processTask()
{
    while (!m_taskQ.empty())
    {
        // 加锁
        m_mutex.lock();
        ChannelElement *cement = m_taskQ.front();
        m_taskQ.pop();
        // 解锁
        m_mutex.unlock();
        Channel *channel = cement->channel;

        if (cement->type == Elemtype::ADD)
        {
            add(channel);
        }
        else if (cement->type == Elemtype::DELETE)
        {
            remove(channel);
        }
        else if (cement->type == Elemtype::MODIFY)
        {
            modify(channel);
        }
        delete (cement);
    }
    return 0;
}

int EventLoop::add(Channel *channel)
{
    int fd = channel->getfd();
    int ret;
    if (m_channelmap.find(fd) == m_channelmap.end()) // 没找到，map中没有这个节点
    {
        m_channelmap.insert(make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        ret = m_dispatcher->add();
    }
    return ret;
}

int EventLoop::remove(Channel *channel)
{
    int fd = channel->getfd();
    int ret;
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::modify(Channel *channel)
{
    int fd = channel->getfd();
    int ret;
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::freeChannel(Channel *channel)
{
    auto it = m_channelmap.find(channel->getfd());
    if (it != m_channelmap.end())
    {
        m_channelmap.erase(it);
        close(channel->getfd());
        delete channel;
        return 0;
    }
    return -1;
}

int EventLoop::readMessage()
{
    char buf[128];
    read(m_sv[1], buf, sizeof(buf));
    return 0;
}

int EventLoop::readLocalMessage(void *arg)
{
    struct EventLoop *evloop = (struct EventLoop *)arg;
    char buf[128];
    read(evloop->m_sv[1], buf, sizeof(buf));
    return 0;
}

void EventLoop::taskWakeup()
{
    write(m_sv[0], "Wake up now!", strlen("Wake up now!"));
}
