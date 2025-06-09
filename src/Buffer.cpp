//#define _GNU_SOURCE
#include"Buffer.h"
#include<stdio.h>
#include<sys/uio.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>

Buffer::Buffer(int size) : m_capacity(size)
{
    m_data = (char*)malloc(size);
    bzero(m_data,size);
}

Buffer::~Buffer()
{
    if(m_data != nullptr)
    {
        free(m_data);
    }
}

void Buffer::extendRoom(int size)
{
    //剩余可写容量足够，无需扩容  可写 > size
    if(writeableSize() >= size)
    {
        return;
    }
    //容量合并后足够，无需扩容  可写 + 已读 > size
    else if(m_readPos + writeableSize() >= size)
    {
        //获取可读，即移动缓冲区的大小
        int readable = readableSize();
        //移动内存
        memcpy(m_data,m_data + m_readPos,readable);
        //更新位置
        m_readPos = 0;
        m_writePos = readable;
    }
    //都不行，需要扩容
    else
    {
        //扩容
        void* temp = realloc(m_data,m_capacity + size);
        memset((char*)temp + m_capacity,0,size);
        //更新位置
        m_data = (char*)temp;
        m_capacity += size;
    }
}

int Buffer::writeableSize()
{
    return m_capacity - m_writePos;
}

int Buffer::readableSize()
{
    return m_writePos - m_readPos;
}

int Buffer::appendString(const char *data, int size)
{
    if(data == nullptr || size <= 0)
    {
        return -1;
    }
    //调用扩容函数（不一定扩）
    extendRoom(size);
    //写入数据
    memcpy(m_data + m_writePos,data,size);
    //更新位置
    m_writePos += size;

    return 0;
}

int Buffer::appendString(const char *data)
{
    int size = strlen(data);  //strlen 找的是字符串结尾处的'\0',但是有些字符串的中间位置就有'\0',这是这个函数的弊端
    int ret = appendString(data,size);
    return ret;
}

int Buffer::appendString(const string data)
{
    int ret = appendString(data.data());
    return ret;
}

int Buffer::socketRead(int fd)
{
    //使用readv   这个函数可以把数据读到多个缓冲区，第一个缓冲区满了，就读第二个
    struct iovec iov[2];
    //初始化数组
    int writeable = writeableSize();
    iov[0].iov_base = m_data + m_writePos;
    iov[0].iov_len = writeable;
    char* temp = (char*)malloc(40960);
    iov[1].iov_base = temp;
    iov[1].iov_len = 40960;
    //调用readv
    int ret = readv(fd,iov,2);  //返回读到的字节数
    if(ret == -1)
    {
        perror("readv");
        return -1;
    }
    else if(ret <= writeable)
    {
        m_writePos += ret;
    }
    else
    {   
        m_writePos = m_capacity;
        appendString(temp,ret - writeable);
    }
    free(temp);
    return ret;
}

char *Buffer::findCRLF()
{
    // strstr --> 大字符串中匹配子字符串(遇到\0结束) char *strstr(const char *haystack, const char *needle);
    // memmem --> 大数据块中匹配子数据块(需要指定数据块大小)
    // void *memmem(const void *haystack, size_t haystacklen,
    //      const void* needle, size_t needlelen);
    char*ptr = (char*)memmem(m_data + m_readPos,readableSize(),"\r\n",2);
    return ptr;
}

int Buffer::sendData(int socket)
{
    //检测有无可发数据
    int readable = readableSize();
    //发送数据
    if(readable > 0)
    {
        int count = send(socket,m_data + m_readPos,readable,MSG_NOSIGNAL);
        if(count > 0)
        {
            m_readPos += count;
            usleep(1);
        }
        else if(count == -1)
        {
            if(errno == EAGAIN)
            {
                return 0;
            }
            perror("send");
            return -1;
        }
        return count;
    }
    return 0;
}
