#include "Server.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Acceptor.h"
#include <functional>
#include <string.h>
#include <unistd.h>
#include "Connection.h"
#define READ_BUFFER 1024

Server::Server(EventLoop *_loop) : loop(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(loop);
    //&Server::newConnection：这是一个指向 Server 类成员函数的指针，表示要调用的函数。
    //this：指向当前 Server 对象的指针，确保在回调时可以访问该对象的成员。
    //std::placeholders::_1：占位符，表示回调函数会接收一个参数，具体是在调用时传入的（在这里是 Socket* 类型）。
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);
}

Server::~Server(){
    delete acceptor;
}

//会从acceptor类的acceptConnection函数跳回到Server类的newConnection函数
//因为conn变量是在Server类维护
//这个函数是发生新连接时会调用的函数
void Server::newConnection(Socket *sock){
    Connection *conn = new Connection(loop, sock);
    std::function<void(Socket*)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
    conn->setDeleteConnectionCallback(cb);
    connections[sock->getFd()] = conn;
}
void Server::deleteConnection(Socket * sock){
    Connection *conn = connections[sock->getFd()];
    connections.erase(sock->getFd());
    delete conn;
}