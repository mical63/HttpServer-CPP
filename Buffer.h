#pragma once
#include<string>
using namespace std;

// 定义缓冲区结构体
struct Buffer
{
public:
    Buffer(int size);
    ~Buffer();

    // 扩容
    void extendRoom(int size);

    // 得到剩余的可写的内存容量
    int writeableSize();

    // 得到剩余的可读的内存容量
    int readableSize();

    // 写内存
    // 1. 直接写
    int appendString(const char *data, int size);
    int appendString(const char *data);
    int appendString(const string data);

    // 2. 接收套接字数据
    int socketRead(int fd);

    // 根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
    char *findCRLF();

    // 发送数据
    int sendData(int socket);

    //获取读数据的起始位置
    inline char* data()
    {
        return m_data + m_readPos;
    }

    //获取变化的readpos
    inline int readPosIncrease(int count)
    {
        m_readPos += count;
        return m_readPos;
    }
private:
    // 指向缓冲区地址的指针
    char *m_data;
    // 缓冲区的容量
    int m_capacity;
    // 读位置
    int m_readPos = 0;
    // 写位置
    int m_writePos = 0;
};