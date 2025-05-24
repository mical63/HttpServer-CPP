#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
#include "TcpConnection.h"
#include <functional>

// 定义结构体
class TcpServer
{
public:
    TcpServer(unsigned short port, int threadNum);
    ~TcpServer();

    // 初始化listener
    void setlistener();

    // 启动服务器
    void tcpServerRun();

    //建立连接
    int acceptConnection();
private:
    // 主反应堆
    struct EventLoop *m_mainloop;
    // 线程池线程数量
    int m_threadNUm;
    // 线程池
    struct ThreadPool *m_pool;

    int m_lfd;
    unsigned short m_port;
};
