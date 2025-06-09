#include"ThreadPool.h"
#include<assert.h>
#include"Log.h"

ThreadPool::ThreadPool(EventLoop *mainevLoop, int count)
{
    m_isStart = false;

    m_threadNum = count;

    m_index = 0;

    m_workerthreads.clear();

    m_mainevloop = mainevLoop;
}

ThreadPool::~ThreadPool()
{
    for(auto item : m_workerthreads)
    {
        delete item;
    }
}

void ThreadPool::run()
{
    assert(!m_isStart);
    if(this_thread::get_id() != m_mainevloop->gettid())  //只能主线程启动
    {
        exit(1);
    }
    m_isStart = true;
    if(m_threadNum > 0)
    {
        for(int i = 0;i < m_threadNum;i++)
        {
            WorkerThread* workerThread = new WorkerThread(i);
            workerThread->run();
            m_workerthreads.push_back(workerThread);
        }
    }
    Debug("线程池启动了。。。");
}

EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);
    if(this_thread::get_id() != m_mainevloop->gettid())  //只能主线程调用
    { 
        exit(1);
    }
    //取出某一个线程的反应堆实例
    struct EventLoop* evloop = m_mainevloop;
    if(m_threadNum > 0)
    {
        evloop = m_workerthreads[m_index]->getEventLoop();
        m_index = ++m_index % m_threadNum;
    }
    return evloop;
}
