#include "Channel.h"

//初始化
Channel::Channel(int fd, FDEvent events, handleFunc read_cb, handleFunc write_cb, handleFunc destroy_cb, void *arg)
{
    m_fd = fd;
    m_events = (int)events;
    m_arg = arg;
    this->read_cb = read_cb;    
    this->write_cb = write_cb;
    this->destroy_cb = destroy_cb;
}

// 修改fd的写事件(检测 or 不检测)
void Channel::writeEventEnable(bool flag)
{
    if(flag == true)
    {
        m_events |= (int)FDEvent::writeEvents;
    }
    else
    {
        m_events &= ~(int)FDEvent::writeEvents;
    }
}

// 判断是否需要检测文件描述符的写事件
bool Channel::isWriteEventEnable()
{
    return m_events & (int)FDEvent::writeEvents;
}
