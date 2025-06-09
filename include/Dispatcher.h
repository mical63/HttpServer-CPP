#pragma once
#include<string.h>
#include "Channel.h"     //封装了文件描述符 事件 回调函数
#include "EventLoop.h"
#include <string>
using namespace std;

class EventLoop;

class Dispatcher
{
public:
    Dispatcher(EventLoop* evloop);
    virtual ~Dispatcher();
    //添加
    virtual int add() = 0;
    //删除
    virtual int remove() = 0;
    //修改
    virtual int modify() = 0;
    //事件监测
    virtual int dispatch(int timeout) = 0;     //单位：s
    //更新channel
    inline void setChannel(Channel* channel)
    {
        this->m_channel = channel;
    }
protected:
    EventLoop* m_evloop;
    Channel* m_channel;
    string m_name = string();
};