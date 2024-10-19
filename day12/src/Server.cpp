#include "Server.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include <functional>
#include <string.h>
#include <unistd.h>
#include "Connection.h"
#include "ThreadPool.h"
#include "EventLoop.h"
#define READ_BUFFER 1024

Server::Server(EventLoop *_loop) : mainReactor(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(mainReactor);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);
    int size = std::thread::hardware_concurrency();
    thpool = new ThreadPool(size);
    for(int i = 0; i < size; ++i){
        subReactors.push_back(new EventLoop());
    }

    for(int i = 0; i < size; ++i){
        std::function<void()> sub_loop = std::bind(&EventLoop::loop, subReactors[i]);
        thpool->add(sub_loop);
    }
}

Server::~Server(){
    delete acceptor;
    delete thpool;
}

//会从acceptor类的acceptConnection函数跳回到Server类的newConnection函数
//因为conn变量是在Server类维护
//这个函数是发生新连接时会调用的函数
void Server::newConnection(Socket *sock){
    if(sock->getFd() != -1){
        int random = sock->getFd() % subReactors.size();
        Connection *conn = new Connection(subReactors[random], sock);
        std::function<void(int)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
        conn->setDeleteConnectionCallback(cb);
        connections[sock->getFd()] = conn;
    }
}
void Server::deleteConnection(int sockfd){
    if(sockfd != -1){
        auto it = connections.find(sockfd);
        if(it != connections.end()){
            Connection *conn = connections[sockfd];
            connections.erase(sockfd);
            // close(sockfd);       //正常
            delete conn;         //会Segmant fault
        }
    }
}