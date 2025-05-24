#pragma once
#include<functional>
using namespace std;

//定义回调函数指针类型
//typedef int(*handleFunc)(void* arg);
//using handleFunc = int(*)(void* arg);

//强类型枚举 事件类型
enum class FDEvent
{
    readEvents = 0x02,
    writeEvents = 0x04
};

//channel结构体
class Channel
{
public:
    // 可调用对象包装器打包的是什么? 1. 函数指针 2. 可调用对象(可以像函数一样使用)
    // 最终得到了地址, 但是没有调用
    using handleFunc = function<int(void*)>;
    //构造  初始化一个channel对象
    Channel(int fd,FDEvent events,handleFunc read_cb,handleFunc write_cb,handleFunc destroy_cb,void* arg);
    // 修改fd的写事件(检测 or 不检测)
    void writeEventEnable(bool flag);
    // 判断是否需要检测文件描述符的写事件
    bool isWriteEventEnable();
    //回调函数
    handleFunc read_cb;
    handleFunc write_cb;
    handleFunc destroy_cb;
    //get
    inline int getfd()
    {
        return this->m_fd;
    }
    inline int getEvents()
    {
        return this->m_events;
    }
    inline const void* getarg()
    {
        return this->m_arg;
    }
private:
    //文件描述符
    int m_fd;
    //事件
    int m_events;
    //回调函数的参数
    void* m_arg;
};
