#pragma once
#include "Channel.h"     //封装了文件描述符 事件 回调函数
#include "EventLoop.h"
#include "Dispatcher.h"
#include<string>

class EpollDispatcher : public Dispatcher
{
public:
    EpollDispatcher(EventLoop* evloop);
    ~EpollDispatcher();
    //添加
    int add() override; //override:1.明确意图 2.编译时检查派生类中的函数签名与基类中的虚函数是否匹配
    //删除
    int remove() override;
    //修改
    int modify() override;
    //事件监测
    int dispatch(int timeout = 2) override;     //单位：s
private:
    int EpollCtl(int op);
private:
    int m_epfd;
    //有效cfd数组
    struct epoll_event* m_evs;  
    //最大监听数
    int m_maxNode = 520;
};