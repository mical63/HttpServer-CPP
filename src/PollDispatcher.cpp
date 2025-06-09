#include "PollDispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

PollDispatcher::PollDispatcher(EventLoop* evloop) : Dispatcher(evloop)
{
    m_maxfd = 0;
    this->m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; i++)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[] m_fds;
}

int PollDispatcher::add()
{
    int i = 0;
    for (; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].fd = m_channel->getfd();
            if (m_channel->getEvents() & (int)FDEvent::readEvents)
            {
                m_fds[i].events |= POLLIN;
            }
            if (m_channel->getEvents() & (int)FDEvent::writeEvents)
            {
                m_fds[i].events |= POLLOUT;
            }
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

// 删除
int PollDispatcher::remove()
{
    int i = 0;
    for (; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == m_channel->getfd())
        {
            m_fds[i].fd = -1;
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            break;
        }
    }
    // 通过channel释放资源
    m_channel->destroy_cb(const_cast<void *>(m_channel->getarg()));
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

// 修改
int PollDispatcher::modify()
{
    int events = 0;
    if (m_channel->getEvents() & (int)FDEvent::readEvents)
    {
        events |= POLLIN;
    }
    if (m_channel->getEvents() & (int)FDEvent::writeEvents)
    {
        events |= POLLOUT;
    }
    int i = 0;
    for (; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == m_channel->getfd())
        {
            m_fds[i].events = events;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

// 事件监测
int PollDispatcher::dispatch(int timeout)
{
    int num = poll(m_fds, m_maxfd + 1, timeout * 1000);
    if (num == -1)
    {
        perror("poll error");
        exit(1);
    }
    for (int i = 0; i <= m_maxfd; i++)
    {
        if (m_fds[i].fd == -1)
        {
            continue;
        }
        if (m_fds[i].revents & POLLIN)
        {
            // eventActivate(evloop, data->fds[i].fd, readEvents);
            m_evloop->eventActivate(m_fds[i].fd,(int)FDEvent::readEvents);
        }
        if (m_fds[i].revents & POLLOUT)
        {
            // eventActivate(evloop, data->fds[i].fd, writeEvents);
            m_evloop->eventActivate(m_fds[i].fd,(int)FDEvent::writeEvents);
        }
    }
    return 0;
}
