#pragma once
#include <sys/epoll.h>
#include <vector>

class Epoll
{
private:
    int epfd;
    struct epoll_event *events;
public:
    Epoll();
    ~Epoll();

    void addFd(const int fd, const uint32_t op);
    std::vector<epoll_event> poll(const int timeout = -1);
};