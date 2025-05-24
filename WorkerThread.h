#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

// 定义子线程结构体
struct WorkerThread
{
public:
    WorkerThread(int index);
    ~WorkerThread();

    // 启动线程
    void run();

    inline EventLoop *getEventLoop()
    {
        return m_evloop;
    }

private:
    // 子线程的回调函数
    void running(); 

private :
    // 线程实例
    thread *m_thread;
    // 线程id
    thread::id m_tid;
    // 线程名
    string m_tname;
    // 互斥锁                  锁反应堆
    mutex m_mutex;
    // 条件变量                反应堆不为空
    condition_variable m_cond;
    // 反应堆
    EventLoop *m_evloop;
};
