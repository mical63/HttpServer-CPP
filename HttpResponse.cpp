//#define _GNU_SOURCE  // 必须在包含头文件之前定义
#include"HttpResponse.h"
#include<stdlib.h>
#include <string.h>
#include"Log.h"

HttpResponse::HttpResponse()
{
    m_statuscode = ResponseStatusCode::Unkown;

    m_filename = string();

    m_headers.clear();

    sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
    if(key.empty() || value.empty())
    {
        return;
    }
    m_headers.insert(make_pair(key,value));
}

void HttpResponse::prepareMsg(Buffer * sendBuf, int sockfd)
{
    char tmp[1024] = {0};
    //状态码
    int code = static_cast<int>(m_statuscode);
    //sprintf(tmp,"HTTP/1.1 %d %s\r\n",code,it->second.data());
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
    sendBuf->appendString(tmp); 
    //响应头
    for(auto item : m_headers)
    {
        sprintf(tmp,"%s: %s\r\n",item.first.data(),item.second.data());
        sendBuf->appendString(tmp); 
    }
    m_headers.clear();
    //空行
    sendBuf->appendString("\r\n"); 

    Debug("回应的HTTP消息: %s",sendBuf->data());

#ifndef MSG_SEND_AUTO
    //发送数据
    sendBuf->sendData(sockfd);
#endif

    //回复的数据
    sendDataFunc(m_filename,sendBuf,sockfd);
}
