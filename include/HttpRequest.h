#pragma once
#include "HttpResponse.h"
#include <ctype.h>
#include <map>
#include<functional>
#include<string>
using namespace std;

// 状态
enum class ProcessState : char
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};

// 请求
class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();
    // 重置
    void reset();

    // 获取处理状态
    ProcessState getState();
    // 添加请求头
    void addHeader(const string key, const string value);
    // 根据key得到请求头的value
    string getHeader(const string key);

    char *splitRequestLine(const char *start, const char *end, const char *sub, function<void(string)> callback);
    // 解析请求行
    bool parseHttpRequestLine(Buffer* readBuf);
    // 解析请求头
    bool parseHttpRequestHeader(Buffer *readBuf);
    // 解析完整的请求
    bool parseHttpRequest(Buffer *readbuf, HttpResponse *response, Buffer *sendbuf, int sockfd);
    // 处理http请求协议
    bool processHttpRequest(HttpResponse *response);

    // 发送文件
    void sendFile(const string filename, Buffer *sendbuf, int sockfd);
    // 发送目录
    void sendDir(const string dirname, Buffer *sendbuf, int sockfd);

    // 将字符转换为整形数
    int hexToDec(char c);
    // 解码
    // to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
    string decodeMsg(string from);
    // 获取文件类型
    const string getFileType(const string name);

    inline void setState(ProcessState state)
    {
        m_curState = state;
    }
    inline void setmethod(string method)
    {
        m_method = method;
    }
    inline void seturl(string url)
    {
        m_url = url;
    }

    inline void setversion(string version)
    {
        m_version = version;
    }


private:
    // 方法
    string m_method;
    // 资源
    string m_url;
    // 协议
    string m_version;
    //map
    map<string,string> m_reqHeaders;
    // 当前状态
    ProcessState m_curState;
};
