#include "Channel.h"
#include "EventLoop.h"

Channel::Channel(EventLoop *_loop, int _fd) 
    : loop(_loop), fd(_fd), events(0), ready(0), inEpoll(false), useThreadPool(true){}

Channel::~Channel()
{
}

void Channel::handleEvent(){
    //callback();
    if(ready & (EPOLLIN | EPOLLPRI)){
        if(useThreadPool)       
            loop->addThread(readCallback);
        else
            readCallback();
    }
    if(ready & (EPOLLOUT)){
        if(useThreadPool)       
            loop->addThread(writeCallback);
        else
            writeCallback();
    }
}


void Channel::enableRead(){
    events |= EPOLLIN | EPOLLPRI;
    loop->updateChannel(this);
}

void Channel::enableReading(){
    events |= EPOLLIN | EPOLLET;
    loop->updateChannel(this);
}
void Channel::useET(){
    events |= EPOLLET;
    loop->updateChannel(this);
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


void Channel::setInEpoll(bool _in){
    inEpoll = _in;
}
// void Channel::setEvents(uint32_t _ev){
//     events = _ev;
// }

void Channel::setRevents(uint32_t _ev){
    revents = _ev;
}

void Channel::setCallback(std::function<void()> _cb){
    callback = _cb;
}
void Channel::setReadCallback(std::function<void()> _cb){
    readCallback = _cb;
}
void Channel::setReady(uint32_t _ev){
    ready = _ev;
}
void Channel::setUseThreadPool(bool _useThreadPool){
    useThreadPool=_useThreadPool;
}
