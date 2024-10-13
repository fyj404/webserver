#include "Channel.h"
#include "Epoll.h"

//构造函数接受 Epoll 指针和文件描述符，用于初始化 Channel 实例。events 和 revents 初始为 0，表示暂时不关注任何事件。
Channel::Channel(Epoll *_ep, int _fd) : ep(_ep), fd(_fd), events(0), revents(0), inEpoll(false){

}

Channel::~Channel()
{
}

//使 Channel 开始监听可读事件（EPOLLIN），并调用 Epoll 对象的 updateChannel() 函数，将 Channel 加入 epoll 事件循环。
void Channel::enableReading(){
    events = EPOLLIN | EPOLLET;
    ep->updateChannel(this);
}

int Channel::getFd(){
    return fd;
}

uint32_t Channel::getEvents(){
    return events;
}
uint32_t Channel::getRevents(){
    return revents;
}

bool Channel::getInEpoll(){
    return inEpoll;
}

void Channel::setInEpoll(){
    inEpoll = true;
}

// void Channel::setEvents(uint32_t _ev){
//     events = _ev;
// }

void Channel::setRevents(uint32_t _ev){
    revents = _ev;
}