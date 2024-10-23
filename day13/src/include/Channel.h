#pragma once
#include <string>
#include "Macros.h"
#include <functional>
class Socket;
class EventLoop;
class Channel{

public:
    explicit Channel(EventLoop *loop,int fd);
    ~Channel();
    DISALLOW_COPY_AND_MOVE(Channel);
    void HandleEvent();
    void EnableRead();
    int GetFd()const;
    uint32_t GetListenEvents()const;
    uint32_t GetReadyEvents()const;
    bool GetInEpoll()const;
    void SetInEpoll(bool _in=true);
    void UseET();
    void SetReadyEvents(uint32_t ev);
    void SetReadCallback(std::function<void()>const &callback);
private:
    EventLoop *loop_;
    int fd_;
    uint32_t listen_events_;
    uint32_t ready_events_;
    bool in_epoll_;
    std::function<void()>read_callback_;
    std::function<void()>write_callback_;
};