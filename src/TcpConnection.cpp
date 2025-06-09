#include "TcpConnection.h"
#include "Log.h"

TcpConnection::TcpConnection(int fd, EventLoop *evloop)
{
    m_evloop = evloop;
    m_readBuf = new Buffer(10240);
    m_writeBuf = new Buffer(10240);
    // http
    m_request = new HttpRequest;
    m_response = new HttpResponse;
    m_name = "Connection-" + to_string(fd);
    m_channel = new Channel(fd, FDEvent::readEvents, processRead, processWrite, destroy, this);
    evloop->addTask(m_channel, Elemtype::ADD);
    Debug("建立连接了。。。。");
}

TcpConnection::~TcpConnection()
{
    if (m_readBuf && m_readBuf->readableSize() == 0 && m_writeBuf && m_writeBuf->writeableSize() == 0)
    {
        m_evloop->freeChannel(m_channel);
        delete m_readBuf;
        delete m_writeBuf;
        delete m_request;
        delete m_response;
    }
    Debug("连接断开, 释放资源, gameover, connName: %s", m_name.data());
}

int TcpConnection::processRead(void *arg)
{
    // 接收数据
    TcpConnection *conn = (TcpConnection *)arg;
    // 读数据
    int ret = conn->m_readBuf->socketRead(conn->m_channel->getfd());

    Debug("接收到的http请求数据: %s", conn->m_readBuf->data());

    if (ret > 0)
    {
#ifdef MSG_SEND_AUTO
        // 检测读写事件
        conn->m_channel->writeEventEnable(true);
        conn->m_evloop->addTask(conn->m_channel, Elemtype::MODIFY);
#endif
        // 解析HTTP协议
        bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, conn->m_channel->getfd());
        if (flag == false)
        {
            // 解析失败
            string tmp = "HTTP/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuf->appendString(tmp);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        conn->m_evloop->addTask(conn->m_channel, Elemtype::DELETE);
#endif
#ifndef MSG_SEND_AUTO
    // 断开连接
    conn->m_evloop->addTask(conn->m_channel, Elemtype::DELETE);
#endif
    }

    return 0;
}

int TcpConnection::processWrite(void *arg)
{
    // 接收数据
    TcpConnection *conn = (TcpConnection *)arg;
    // 发送数据
    int count = conn->m_writeBuf->sendData(conn->m_channel->getfd());
    if (count > 0)
    {
        if (conn->m_writeBuf->readableSize() == 0)
        {
            // 检测写事件
            conn->m_channel->writeEventEnable(false);
            conn->m_evloop->addTask(conn->m_channel, Elemtype::MODIFY);
            // 可断开连接
            conn->m_evloop->addTask(conn->m_channel, Elemtype::DELETE);
        }
    }
    return 0;
}

int TcpConnection::destroy(void *arg)
{
    // 接收数据
    TcpConnection *conn = (TcpConnection *)arg;
    if (conn != nullptr)
    {
        delete conn;
    }
    return 0;
}
