#pragma once
#include <sys/epoll.h>
#include <vector>

class Channel;
class Epoll
{
private:
    int epfd;//实例的文件描述符，用于识别 epoll 对象。
    struct epoll_event *events; //指向 epoll_event 结构体数组的指针，用于存储活动事件的信息
public:
    Epoll();
    ~Epoll();

    void addFd(int fd, uint32_t op);
    void updateChannel(Channel*);
    // std::vector<epoll_event> poll(int timeout = -1);
    std::vector<Channel*> poll(int timeout = -1);
};