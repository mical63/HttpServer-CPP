#pragma once
#include"EventLoop.h"
#include"Channel.h"
#include"Buffer.h"
#include"HttpRequest.h"
#include"HttpResponse.h"
#include<stdlib.h>

//#define MSG_SEND_AUTO

class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop* evloop);
    ~TcpConnection();

    static int processRead(void* arg);
    static int processWrite(void* arg);
    static int destroy(void* arg);
private:
    //名字
    string m_name;
    //两个缓冲
    Buffer* m_readBuf;
    Buffer* m_writeBuf;
    //反应堆（子线程提供）
    EventLoop* m_evloop;
    //channel
    Channel* m_channel;
    //http
    HttpRequest* m_request;
    HttpResponse* m_response;
};