#pragma once
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include<functional>
#include<string>
using namespace std;

// 状态码
enum class ResponseStatusCode
{
    Unkown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

// 定义一个函数指针 文件 or 目录
// typedef void (*responseBody)(const char* filename,struct Buffer* sendbuf,int sockfd);

// 回应消息 结构体
class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    // 添加响应头
    void addHeader(const string key, const string value);
    // 组织http响应数据
    void prepareMsg(Buffer *sendBuf, int sockfd);

    inline void setFilename(string name)
    {
        m_filename = name;
    }
    inline void setStatus(ResponseStatusCode status)
    {
        m_statuscode = status;
    }

    //可调用对象包装器
    function<void(const string, struct Buffer*, int)> sendDataFunc;
private:
    // 状态码
    ResponseStatusCode m_statuscode;
    // 文件名
    string m_filename;
    // 消息头
    map<string, string> m_headers;
    // 状态码--状态描述
    map<int, string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"},
    };
};