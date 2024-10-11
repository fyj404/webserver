#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include "util.h"
#include "Epoll.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"
#define MAX_EVENTS 1024
#define READ_BUFFER 1024

// void setnonblocking(int fd){
//     fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
// }
void handleReadEvent(int);

int main() {
    Socket *serv_sock = new Socket();
    InetAddress *serv_addr = new InetAddress("127.0.0.1", 8888);
    serv_sock->bind(serv_addr);
    serv_sock->listen();    
    Epoll *ep = new Epoll();
    serv_sock->setnonblocking();
    Channel *servChannel = new Channel(ep, serv_sock->getFd());
    servChannel->enableReading();
    while(true){
        std::vector<Channel*> activeChannels = ep->poll();
        int nfds = activeChannels.size();
        for(int i = 0; i < nfds; ++i){
            int chfd = activeChannels[i]->getFd();
            if(chfd == serv_sock->getFd()){        //新客户端连接
                InetAddress *clnt_addr = new InetAddress();      //会发生内存泄露！没有delete
                Socket *clnt_sock = new Socket(serv_sock->accept(clnt_addr));       //会发生内存泄露！没有delete
                printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->getFd(), inet_ntoa(clnt_addr->addr.sin_addr), ntohs(clnt_addr->addr.sin_port));
                clnt_sock->setnonblocking();
                Channel *clntChannel = new Channel(ep, clnt_sock->getFd());
                clntChannel->enableReading();
            } else if(activeChannels[i]->getRevents() & EPOLLIN){      //可读事件
                handleReadEvent(activeChannels[i]->getFd());
            } else{         //其他事件，之后的版本实现
                printf("something else happened\n");
            }
        }
    }
    delete serv_sock;
    delete serv_addr;
    return 0;
}

void handleReadEvent(int sockfd){
    char buf[READ_BUFFER];
    while(true){    //由于使用非阻塞IO，读取客户端buffer，一次读取buf大小数据，直到全部读取完毕
        bzero(&buf, sizeof(buf));
        ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
        if(bytes_read > 0){
            printf("message from client fd %d: %s\n", sockfd, buf);
            write(sockfd, buf, sizeof(buf));
        } else if(bytes_read == -1 && errno == EINTR){  //客户端正常中断、继续读取
            //在非阻塞I/O的场景下，可能会因为信号中断（EINTR）导致读取操作中途被打断。
            //在这种情况下，程序不会退出读取，而是会继续循环读取数据。这里的处理方式是输出提示信息，跳过这次中断，继续从read()读取。
            printf("continue reading");
            continue;
        } else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
            //在非阻塞模式下，如果当前没有数据可以读取，read()会返回-1，并且errno被设置为EAGAIN或EWOULDBLOCK。
            //这并不是一个错误，而是告诉我们当前没有更多的数据可以读取了。此时，程序会跳出循环，因为已经读取完了当前客户端可提供的数据。
            //EAGAIN 和 EWOULDBLOCK 基本上没有区别，它们在大多数系统上是等价的，但它们出现的场景和历史背景稍有不同
            //在许多现代系统中（例如Linux），两者被定义为相同的值，EWOULDBLOCK只是EAGAIN的别名。因此，当你处理非阻塞I/O操作时，通常只需要检查一个即可。
            printf("finish reading once, errno: %d\n", errno);
            break;
        } else if(bytes_read == 0){  //EOF，客户端断开连接
            printf("EOF, client fd %d disconnected\n", sockfd);
            close(sockfd);   //关闭socket会自动将文件描述符从epoll树上移除
            break;
        }
    }
}