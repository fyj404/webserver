#pragma once
#include <sys/epoll.h>

class Epoll;
class Channel
{
    /**
     * Epoll *ep：指向 Epoll 对象的指针，用于管理 epoll 事件循环。
        int fd：文件描述符，用于标识需要监控的 I/O 对象，可能是套接字、文件等。
        uint32_t events：当前关注的事件类型，比如可读、可写事件等。这里的 events = EPOLLIN | EPOLLET; 表示关注“可读事件” (EPOLLIN) 和“边缘触发” (EPOLLET) 模式。
        uint32_t revents：实际发生的事件类型，由 epoll 返回。
        bool inEpoll：表示该 Channel 是否已经被加入到 epoll 监听列表中。
     */
private:
    Epoll *ep;
    int fd;
    uint32_t events;
    uint32_t revents;
    bool inEpoll;
public:
    Channel(Epoll *_ep, int _fd);
    ~Channel();

    void enableReading();

    int getFd();
    uint32_t getEvents();
    uint32_t getRevents();
    bool getInEpoll();
    void setInEpoll();

    // void setEvents(uint32_t);
    void setRevents(uint32_t);
};