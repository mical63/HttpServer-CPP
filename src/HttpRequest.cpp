//#define _GNU_SOURCE // 必须在包含头文件之前定义，必须在所有头文件之前写这个宏
#include<iostream>
#include "HttpRequest.h"
#include <stdio.h>
#include "Buffer.h"
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "Log.h"



HttpRequest::HttpRequest()
{
    reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
    m_method = string();
    m_url = string();
    m_version = string();
    m_curState = ProcessState::ParseReqLine;
    m_reqHeaders.clear();
}

ProcessState HttpRequest::getState()
{
    return m_curState;
}

void HttpRequest::addHeader(const string key, const string value)
{
    if (key.empty() || value.empty())
    {
        return;
    }
    m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const string key)
{
    auto it = m_reqHeaders.find(key);
    if (it == m_reqHeaders.end())
    {
        return string();
    }
    return it->second;
}

char *HttpRequest::splitRequestLine(const char *start, const char *end, const char *sub, function<void(string)> callback)
{
    char *space = (char *)end;
    if (sub != nullptr)
    {
        space = (char*)memmem(start, end - start, sub, strlen(sub));
        assert(space != nullptr);
    }
    int len = space - start;
    callback(string(start,len));
    return space + 1;
}

bool HttpRequest::parseHttpRequestLine(Buffer *readBuf)
{
    char *start = readBuf->data();
    char *end = readBuf->findCRLF();
    int linesize = end - start;

    if (linesize > 0)
    {
        auto methodfunc = bind(&HttpRequest::setmethod,this,placeholders::_1);
        start = splitRequestLine(start, end, " ", methodfunc);
        auto urlfunc = bind(&HttpRequest::seturl,this,placeholders::_1);
        start = splitRequestLine(start, end, " ", urlfunc);
        auto versionfunc = bind(&HttpRequest::setversion,this,placeholders::_1);
        splitRequestLine(start, end, NULL, versionfunc);

        // 更新
        readBuf->readPosIncrease(linesize + 2);

        setState(ProcessState::ParseReqHeaders);
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequestHeader(Buffer *readBuf)
{
    char *end = readBuf->findCRLF();
    if (end != NULL)
    {
        char *start = readBuf->data();
        int linesize = end - start;
        // 找": "
        char *middle = (char *)memmem(start, linesize, ": ", 2);
        if (middle != NULL)
        {
            //key: value\r\n
            //012345678910
            int keylen = middle - start;
            int valuelen = end - middle - 2;
            if(keylen > 0 && valuelen > 0)
            {
                string key(start,keylen);
                string value(middle + 2,valuelen);
                addHeader(key, value);
                readBuf->readPosIncrease(linesize + 2);
            }
        }
        else // 读到空行
        {
            readBuf->readPosIncrease(2);
            setState(ProcessState::ParseReqDone);
        }
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequest(Buffer *readbuf, HttpResponse *response, Buffer *sendbuf, int sockfd)
{
    bool flag = true;
    while (m_curState != ProcessState::ParseReqDone)
    {
        switch (m_curState)
        {
        case ProcessState::ParseReqLine:
            flag = parseHttpRequestLine(readbuf);
            break;
        case ProcessState::ParseReqHeaders:
            flag = parseHttpRequestHeader(readbuf);
            break;
        case ProcessState::ParseReqBody:
            break;
        default:
            break;
        }
        if (flag == false)
        {
            return false;
        }
        if (m_curState == ProcessState::ParseReqDone)
        {
            // 1. 根据解析出的原始数据, 对客户端的请求做出处理
            processHttpRequest(response);
            
            // 2. 组织响应数据并发送给客户端
            response->prepareMsg(sendbuf, sockfd);
        }
    }
    m_curState = ProcessState::ParseReqLine;
    //reset();
    return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse *response)
{
    // get
    if (m_method.compare("GET") != 0)
    {
        printf("3...\n");
        return false;
    }
    m_url = decodeMsg(m_url);
    // 获取文件路径
    const char *file = NULL;
    if (strcmp(m_url.data(), "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = m_url.data() + 1;
    }
    printf("file = %s\n",file);
    // 文件状态
    struct stat st;
    int ret = stat(file, &st);
    // 不存在
    if (ret == -1)
    {
        // 发送404
        // 状态码
        response->setStatus(ResponseStatusCode::NotFound);
        response->setFilename("404.html");
        // 响应头
        response->addHeader("Content-type", getFileType(".html"));
        // 回应内容
        auto obj = bind(&HttpRequest::sendFile,this,placeholders::_1,placeholders::_2,placeholders::_3);
        response->sendDataFunc = obj;
        return false;
    }
    
    response->setFilename(file);
    response->setStatus(ResponseStatusCode::OK);
    // 目录
    if (S_ISDIR(st.st_mode))
    {
        response->addHeader("Content-type", getFileType(".html"));
        auto obj = bind(&HttpRequest::sendDir,this,placeholders::_1,placeholders::_2,placeholders::_3);
        response->sendDataFunc = obj;
    }
    // 文件
    else
    {
        //char tmp[16] = {0};
        //sprintf(tmp, "%ld", st.st_size);
        response->addHeader("Content-type", getFileType(file));
        response->addHeader("Content-length", to_string(st.st_size));
        auto obj = bind(&HttpRequest::sendFile,this,placeholders::_1,placeholders::_2,placeholders::_3);
        response->sendDataFunc = obj;
    }
    return false;
}

void HttpRequest::sendFile(const string filename, Buffer *sendbuf, int sockfd)
{
    Debug("要发文件了。。。");
    // 打开文件
    //printf("filename: %s\n", filename);
    cout << "filename: " << filename << endl;
    int fd = open(filename.data(), O_RDONLY);
    // assert(fd > 0);
    if (fd < 0)
    {
        perror("open error\n");
        exit(1);
    }
    // 发送文件
    char buf[1024] = {0};
#if 1
    while (1)
    {
        int ret = read(fd, buf, sizeof(buf));
        if (ret > 0)
        {
            // send(cfd,buf,ret,0);
            sendbuf->appendString(buf, ret);
#ifndef MSG_SEND_AUTO
            // 发送数据
            sendbuf->sendData(sockfd);
#endif
            // usleep(10); // 防止服务器发的太快，客户端解析不过来
        }
        else if (ret == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read error");
        }
    }
    close(fd);
#else
    off_t offset;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size)
    {
        int ret = sendfile(cfd, fd, &offset, size - offset);
        if (ret == -1)
        {
            perror("send file error\n");
        }
    }
    close(fd);
#endif
}

void HttpRequest::sendDir(const string dirname, Buffer *sendbuf, int sockfd)
{
    Debug("要发目录了。。。");
    // 遍历目录
    struct dirent **namelist; // 指向的是一个指针数组 struct dirent* tmp[]
    int num = scandir(dirname.data(), &namelist, NULL, alphasort);
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirname.data());
    for (int i = 0; i < num; i++)
    {
        // char cwd[1024] = {0};
        // 获取文件名
        char *name = namelist[i]->d_name;
        // 跳过隐藏文件
        if (strcmp(name, ".") == 0)
        {
            continue;
        }
        if (strcmp(name, "..") == 0)
        {
            continue;
        }
        // 判断文件类型
        struct stat st;
        char subPath[1024] = {0};
        // if (strcmp(dirname, "./") == 0)
        // {
        //     sprintf(subPath, "%s%s", dirname, name);
        // }
        // else
        // {
        //     sprintf(subPath, "%s%s", dirname, name);
        // }
        // sprintf(subPath, "%s/%s", dirname, name);
        sprintf(subPath, "%s%s", dirname.data(), name);
        printf("subPath: %s,dirname: %s, name: %s\n", subPath, dirname.data(), name);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode)) // 目录
        {
            // sprintf(buf + strlen(buf),"<tr><td><a href=\"%s\">%s/</a></td><td>%ld</td></tr>",subPath,name,st.st_size);
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        if (S_ISREG(st.st_mode)) // 文件
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        // send(cfd,buf,strlen(buf),0);
        sendbuf->appendString(buf);
#ifndef MSG_SEND_AUTO
        // 发送数据
        sendbuf->sendData(sockfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf + strlen(buf), "</table></body></html>");
    // send(cfd,buf,strlen(buf),0);
    sendbuf->appendString(buf);
#ifndef MSG_SEND_AUTO
    // 发送数据
    sendbuf->sendData(sockfd);
#endif
    free(namelist);
    return;
}

int HttpRequest::hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

string HttpRequest::decodeMsg(string msg)
{
    string str = string();
    const char* from = msg.data();
    for (; *from != '\0'; ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            str.append(1, *from);
        }
    }
    str.append(1, '\0');
    return str;
}

const string HttpRequest::getFileType(const string name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char *dot = strrchr(name.data(), '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8"; // 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
