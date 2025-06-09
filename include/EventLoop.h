#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <sys/socket.h>
#include <queue>
#include <map>
#include <mutex>

// 枚举处理方式 强类型枚举
enum class Elemtype : char
{
    ADD,
    DELETE,
    MODIFY
}; // 以char类型存储，可以节省空间

// 定义任务队列的节点结构体
struct ChannelElement
{
    Elemtype type; // 处理方式
    Channel *channel;
};

class Dispatcher; // 如果两个结构体互相包含。可以提前做一个声明，就不报错了

class EventLoop
{
public:
    EventLoop();
    EventLoop(const string threadName);
    ~EventLoop();

    // 启动反应堆模型
    int run();

    // 处理别激活的文件fd
    int eventActivate(int fd, int event);

    // 向任务队列中添加任务
    int addTask(Channel *channel, Elemtype type);

    // 处理任务
    int processTask();

    // 处理dispatcher中的节点
    int add(Channel *channel);
    int remove(Channel *channel);
    int modify(Channel *channel);

    // 释放channel
    int freeChannel(Channel *channel);

    //sv[1]  读数据
    int readMessage();  //成员函数  可调用对象包装器
    static int readLocalMessage(void* arg);   //静态函数  函数指针

    inline thread::id gettid()
    {
        return m_tid;
    }

private:
    //sv[0] 写数据
    void taskWakeup();

private:
    // 判断Eventloop是否运行
    bool m_isQuit;
    // 父类指针，可自行选择epoll poll select
    Dispatcher *m_dispatcher;
    // 任务队列
    queue<ChannelElement *> m_taskQ;
    // map
    map<int, Channel *> m_channelmap;
    // 线程id name
    thread::id m_tid;
    string m_tname;
    // 互斥锁      锁任务队列
    mutex m_mutex;
    // 存储用于本进程内线程通信的fd，以便主线程唤醒子线程
    int m_sv[2];
};
