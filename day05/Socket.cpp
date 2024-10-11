#include "Socket.h"
#include "InetAddress.h"
#include "util.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

Socket::Socket() : fd(-1){
    fd = socket(AF_INET, SOCK_STREAM, 0);
    errif(fd == -1, "socket create error");
}
Socket::Socket(int _fd) : fd(_fd){
    errif(fd == -1, "socket create error");
}

Socket::~Socket(){
    if(fd != -1){
        close(fd);
        fd = -1;
    }
}

void Socket::bind(const InetAddress *addr)const{
    //将套接字与特定地址绑定。使用 ::bind 调用全局 bind 函数。
    //addr 是指向 InetAddress 对象的指针，使用其 addr 和 addr_len 进行绑定
    //这个表达式的整体流程如下：
    //首先，通过 addr->addr 访问 InetAddress 实例的 addr 成员（通常为 sockaddr_in 类型）。
    //然后，将这个 addr 成员的地址转换为 sockaddr* 类型指针
    errif(::bind(fd, (sockaddr*)&addr->addr, addr->addr_len) == -1, "socket bind error");
}

void Socket::listen()const{
    //listen：将套接字设置为监听状态，接受连接请求。SOMAXCONN 表示系统可以排队的最大连接数。
    errif(::listen(fd, SOMAXCONN) == -1, "socket listen error");
}
void Socket::setnonblocking(){
    //setnonblocking：将套接字设置为非阻塞模式。
    //fcntl(fd, F_GETFL) 获取当前的文件状态标志，然后使用 F_SETFL 设置新的标志，添加 O_NONBLOCK 表示非阻塞模式。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int Socket::accept(InetAddress *addr){
    //accept：接受一个连接请求。
    //addr 用于存储连接的客户端地址。
    //返回新的客户端套接字文件描述符 clnt_sockfd，如果失败则通过 errif 报告错误。
    int clnt_sockfd = ::accept(fd, (sockaddr*)&addr->addr, &(addr->addr_len));
    errif(clnt_sockfd == -1, "socket accept error");
    return clnt_sockfd;
}

int Socket::getFd()const{
    return fd;
}