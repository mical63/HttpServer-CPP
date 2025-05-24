#include "SelectDispatcher.h"
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>

SelectDispatcher::SelectDispatcher(EventLoop *evloop) : Dispatcher(evloop)
{
    FD_ZERO(&m_rdSet);
    FD_ZERO(&m_wrSet);
    m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

// 添加
int SelectDispatcher::add()
{
    if (m_channel->getfd() >= m_maxnode)
    {
        return -1;
    }
    setFdSet();
    return 0;
}

// 删除
int SelectDispatcher::remove()
{
    if (m_channel->getfd() >= m_maxnode)
    {
        return -1;
    }
    clearFdSet();
    // 通过channel释放资源
    m_channel->destroy_cb(const_cast<void*>(m_channel->getarg()));
    return 0;
}

// 修改
int SelectDispatcher::modify()
{
    if (m_channel->getfd() >= m_maxnode)
    {
        return -1;
    }
    // 清除原事件，无论读写
    FD_CLR(m_channel->getfd(), &m_rdSet);
    FD_CLR(m_channel->getfd(), &m_rdSet);
    // 添加新事件
    setFdSet();
    return 0;
}

// 事件监测
int SelectDispatcher::dispatch(int timeout)
{
    struct timeval time;
    time.tv_sec = timeout;
    time.tv_usec = 0;
    fd_set rdtmp = m_rdSet;
    fd_set wrtmp = m_wrSet;
    int num = select(m_maxnode, &rdtmp, &wrtmp, NULL, &time);
    if (num == -1)
    {
        perror("select error\n");
        exit(1);
    }
    int i = 3;
    for (; i < m_maxnode; i++)
    {
        if (FD_ISSET(i, &rdtmp))
        {
            //eventActivate(evloop, i, readEvents);
            m_evloop->eventActivate(i,(int)FDEvent::readEvents);
        }
        if (FD_ISSET(i, &wrtmp))
        {
            //eventActivate(evloop, i, writeEvents);
            m_evloop->eventActivate(i,(int)FDEvent::writeEvents);
        }
    }
    return 0;
}

void SelectDispatcher::setFdSet()
{
    if (m_channel->getEvents() & (int)FDEvent::readEvents)
    {
        FD_SET(m_channel->getfd(), &m_rdSet);
    }
    if (m_channel->getEvents() & (int)FDEvent::writeEvents)
    {
        FD_SET(m_channel->getfd(), &m_wrSet);
    }
}

void SelectDispatcher::clearFdSet()
{
    if (m_channel->getEvents() & (int)FDEvent::readEvents)
    {
        FD_CLR(m_channel->getfd(), &m_rdSet);
    }
    if (m_channel->getEvents() & (int)FDEvent::writeEvents)
    {
        FD_CLR(m_channel->getfd(), &m_wrSet);
    }
}
