#include "WorkerThread.h"
#include <stdio.h>

WorkerThread::WorkerThread(int index)
{
    m_thread = nullptr;

    m_tid = thread::id();

    m_tname = "SunThread--" + to_string(index);

    m_evloop = nullptr;
}
WorkerThread::~WorkerThread()
{
}

// 启动线程
void WorkerThread::run()
{
    // 创建子线程
    m_thread = new thread(&WorkerThread::running,this);
    // 阻塞主线程  防止evloop没创建出来，这个函数就返回
    //m_mutex.lock();  //双重加锁，导致死锁
    unique_lock<mutex> locker(m_mutex);   //这里已有加锁操作
    while (m_evloop == nullptr)
    {
        m_cond.wait(locker);
    }
}

void WorkerThread::running()
{
    //printf("子线程正在启动。。。\n");
    // 创建反应堆
    m_mutex.lock();
    m_evloop = new EventLoop(m_tname);
    m_mutex.unlock();
    // 唤醒主线程
    m_cond.notify_one();
    // 启动反应堆
    m_evloop->run();
}
