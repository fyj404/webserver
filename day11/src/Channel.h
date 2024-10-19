#pragma once
#include <sys/epoll.h>
#include <functional>
class EventLoop;
class Channel
{
private:
    EventLoop *loop;
    int fd;
    uint32_t events;
    uint32_t revents;
    uint32_t ready;
    bool useThreadPool;
    bool inEpoll;
    std::function<void()> callback;
    std::function<void()> readCallback;
    std::function<void()> writeCallback;
public:
    Channel(EventLoop *_loop, int _fd);
    ~Channel();

    void handleEvent();
    void enableReading();
    void enableRead();

    int getFd();
    uint32_t getEvents();
    uint32_t getRevents();
    bool getInEpoll();
    void useET();
    // void setEvents(uint32_t);
    void setRevents(uint32_t);
    void setCallback(std::function<void()>);

    void setReady(uint32_t);
    void setReadCallback(std::function<void()>);
    void setUseThreadPool(bool _useThreadPool=true);
    void setInEpoll(bool _in = true);
};