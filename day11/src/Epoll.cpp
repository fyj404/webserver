#include "Epoll.h"
#include "util.h"
#include "Channel.h"
#include <unistd.h>
#include <string.h>

#define MAX_EVENTS 1000

Epoll::Epoll() : epfd(-1), events(nullptr){
    epfd = epoll_create1(0);//创建一个 epoll 实例，返回一个文件描述符，存储在 epfd 中。如果创建失败，调用 errif 抛出错误。
    errif(epfd == -1, "epoll create error");
    events = new epoll_event[MAX_EVENTS];//分配一个数组，用于存储最多 MAX_EVENTS 个活动事件的信息。
    bzero(events, sizeof(*events) * MAX_EVENTS);
}

Epoll::~Epoll(){
    if(epfd != -1){
        close(epfd);
        epfd = -1;
    }
    delete [] events;
}

// void Epoll::addFd(int fd, uint32_t op){
//     struct epoll_event ev;//定义一个 epoll_event 结构体用于描述要添加的事件。
//     bzero(&ev, sizeof(ev));
//     ev.data.fd = fd;//将文件描述符赋值给 ev 的 data 成员，以便后续获取事件时能找到对应的文件描述符。
//     ev.events = op;//设置要监听的事件类型（如可读、可写等）。
//     errif(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add event error");
//     //调用 epoll_ctl 将文件描述符添加到 epoll 实例中。如果失败，调用 errif 抛出错误。
// }
std::vector<Channel*> Epoll::poll(int timeout){
    //阻塞等待事件发生，返回活动的文件描述符数量。
    std::vector<Channel*> activeChannels;
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, timeout);
    errif(nfds == -1, "epoll wait error");
    for(int i = 0; i < nfds; ++i){
        //将 events[i].data.ptr 强制转换为 Channel*，并将活动事件设置到相应的 Channel 中。
        //将每个活动的 Channel 添加到 activeChannels 向量中，以便后续处理。
        Channel *ch = (Channel*)events[i].data.ptr;

        
        ch->setReady(events[i].events);
        
        activeChannels.push_back(ch);
    }
    return activeChannels;
}

void Epoll::updateChannel(Channel *channel){
    int fd = channel->getFd();
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.data.ptr = channel;
    ev.events = channel->getEvents();
    if(!channel->getInEpoll()){
        //如果不在，使用 EPOLL_CTL_ADD 添加通道。
        errif(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
        channel->setInEpoll();
        // debug("Epoll: add Channel to epoll tree success, the Channel's fd is: ", fd);
    } else{
        //如果已在，使用 EPOLL_CTL_MOD 修改通道的事件。
        errif(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1, "epoll modify error");
        // debug("Epoll: modify Channel in epoll tree success, the Channel's fd is: ", fd);
    }
}
void Epoll::deleteChannel(Channel *channel){
    int fd = channel->getFd();
    errif(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1, "epoll delete error");
    channel->setInEpoll(false);
}