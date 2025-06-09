#include"TcpServer.h"
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include"Log.h"

TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_port = port;
    m_mainloop = new EventLoop;
    m_threadNUm = threadNum;
    m_pool = new ThreadPool(m_mainloop,m_threadNUm);
    setlistener();
}

TcpServer::~TcpServer()
{
    delete m_mainloop;
    delete m_pool;
}

void TcpServer::setlistener()
{
    //初始化m_lfd
    m_lfd = socket(AF_INET,SOCK_STREAM,0);

    if(m_lfd == -1)
    {
        perror("socket");
        exit(1);
    }
    //设置端口服用
    int opt = 1;
    int ret = setsockopt(m_lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(ret == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    //绑定地址结构
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(m_port);
    ser_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    if(ret == -1)
    {
        perror("bind");
        exit(1);
    }
    //设置监听上限
    ret = listen(m_lfd,128);
    if(ret == -1)
    {
        perror("ret");
        exit(1);
    }
}

void TcpServer::tcpServerRun()
{
    //启动线程池
    m_pool->run();
    //添加m_lfd
    auto obj = bind(&TcpServer::acceptConnection,this);
    Channel* channel = new Channel(m_lfd,FDEvent::readEvents,obj,nullptr,nullptr,nullptr);
    //Channel* channel = new Channel(m_lfd,FDEvent::readEvents,acceptConnection,nullptr,nullptr,nullptr);
    m_mainloop->addTask(channel,Elemtype::ADD);
    //启动反应堆模型
    m_mainloop->run();
}

int TcpServer::acceptConnection()
{
    printf("开始监听了。。。。。\n");
    //建立连接
    int cfd = accept(m_lfd,NULL,NULL);

    if(cfd == -1)
    {
        perror("accept");
        exit(1);
    }
    // 从线程池中取出一个子线程的反应堆实例, 去处理这个cfd
    EventLoop* evloop = m_pool->takeWorkerEventLoop();
    // 将cfd放到 TcpConnection中处理
    //TcpConnection* conn = new TcpConnection(cfd, evloop);
    new TcpConnection(cfd, evloop);
    return 0;
}
