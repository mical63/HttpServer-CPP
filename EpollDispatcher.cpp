#include "EpollDispatcher.h"
#include<sys/epoll.h>
using namespace std;
#include<stdio.h>
#include<unistd.h>

EpollDispatcher::EpollDispatcher(EventLoop* evloop) : Dispatcher(evloop)
{
    //创建监听红黑树
    m_epfd = epoll_create(1);

    //printf("epfd = %d\n",m_epfd);

    if(m_epfd == -1)
    {
        perror("epoll_creata error");
        exit(1);
    }
    //为传出数组申请内存
    m_evs = new struct epoll_event[m_maxNode];

    //命名
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[] m_evs;
}

int EpollDispatcher::add()
{
    int ret = EpollCtl(EPOLL_CTL_ADD);
    if(ret == -1)
    {
        perror("epoll add error\n");
        exit(1);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = EpollCtl(EPOLL_CTL_DEL);
    if(ret == -1)
    {
        perror("epoll del error\n");
        exit(1);
    }
    //通过channel释放资源
    m_channel->destroy_cb(const_cast<void*>(m_channel->getarg()));
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = EpollCtl(EPOLL_CTL_MOD);
    if(ret == -1)
    {
        perror("epoll mod error\n");
        exit(1);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    //循环监听
    int num = epoll_wait(m_epfd,m_evs,m_maxNode,timeout * 1000);
    //printf("开始监听了。。。。。\n");
    for(int i = 0;i < num;i++)
    {
        int events = m_evs[i].events;
        int fd = m_evs[i].data.fd;
        if(events & EPOLLERR || events & EPOLLHUP)
        {
            //对端关闭了连接
            //EpollRemove();
            continue;
        }
        if(events & EPOLLIN)
        {
            //printf("读。。。。。\n");
            m_evloop->eventActivate(fd,(int)FDEvent::readEvents);
        }
        if(events & EPOLLOUT)
        {
            m_evloop->eventActivate(fd,(int)FDEvent::writeEvents);
        }
    }
    return 0;
}

int EpollDispatcher::EpollCtl(int op)
{
    //节点操作
    struct epoll_event ev;
    ev.data.fd = m_channel->getfd();
    int events = 0;
    if(m_channel->getEvents() & (int)FDEvent::readEvents)
    {
        events |= EPOLLIN;
    }
    if(m_channel->getEvents() & (int)FDEvent::writeEvents)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;

    int ret = epoll_ctl(m_epfd,op,m_channel->getfd(),&ev);
    return ret;
}
