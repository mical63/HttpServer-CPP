#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdlib.h>
#include <vector>
using namespace std;

// 定义线程池结构体
class ThreadPool
{
public:
    ThreadPool(EventLoop *mainevLoop, int count);
    ~ThreadPool();

    // 启动线程池
    void run();

    // 取出线程池中的某个子线程的反应堆实例
    EventLoop *takeWorkerEventLoop();

private:
    // 是否开启
    bool m_isStart;
    // 线程数量
    int m_threadNum;
    // 子线程数组
    vector<WorkerThread *> m_workerthreads;
    // 即将调用的线程的下标
    int m_index;
    // 主反应堆
    EventLoop *m_mainevloop;
};
