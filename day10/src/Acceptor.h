#pragma once
#include <functional>
//Acceptor 类的主要功能是设置一个监听套接字，并在有新连接到达时通过回调通知外部处理。它封装了套接字的创建、绑定、监听以及事件循环中的事件处理，
//使得服务器能够以事件驱动的方式有效地接受客户端连接。
class EventLoop;
class Socket;
class InetAddress;
class Channel;
class Acceptor
{
private:
    EventLoop *loop; //指向事件循环的指针，负责调度和处理事件。
    Socket *sock; //封装了套接字的对象，用于网络通信。
    Channel *acceptChannel;//用于与 epoll 等事件通知机制交互的通道，监听套接字的可读事件（即有新的连接到达）。
public:
    Acceptor(EventLoop *_loop);
    ~Acceptor();
    void acceptConnection();
    //std::function<void(Socket*)> 是 C++11 引入的一个通用可调用对象类型，
    //它可以存储任何可调用对象，例如普通函数、lambda 表达式、绑定的成员函数、函数对象等。
    //void(Socket*)：表示这个可调用对象接受一个 Socket* 类型的参数，并且没有返回值（即返回类型为 void）。
    std::function<void(Socket*)> newConnectionCallback;
    void setNewConnectionCallback(std::function<void(Socket*)>);
};