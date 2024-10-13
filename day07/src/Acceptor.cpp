#include "Acceptor.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Server.h"

Acceptor::Acceptor(EventLoop *_loop) : loop(_loop)
{
    //创建socket用于监听连接事件
    sock = new Socket();
    addr = new InetAddress("127.0.0.1", 8888);
    sock->bind(addr);
    sock->listen(); 
    sock->setnonblocking();
    acceptChannel = new Channel(loop, sock->getFd());
    std::function<void()> cb = std::bind(&Acceptor::acceptConnection, this);
    //使用 std::bind 将 acceptConnection 方法绑定为回调，传递给 acceptChannel。
    acceptChannel->setCallback(cb);
    //调用 enableReading() 方法，使 acceptChannel 开始监听可读事件，准备接受新连接。
    acceptChannel->enableReading();
}

Acceptor::~Acceptor(){
    delete sock;
    delete addr;
    delete acceptChannel;
}

//acceptConnection 方法被调用时，会触发 newConnectionCallback，
//并传递当前的 sock 对象（表示监听的套接字）。
//该回调通常会在外部定义，用于处理新建立的连接。
void Acceptor::acceptConnection(){
    newConnectionCallback(sock);
}

//该方法允许外部设置一个回调函数，处理新连接的建立，_cb 是接收 Socket* 类型参数的函数。
void Acceptor::setNewConnectionCallback(std::function<void(Socket*)> _cb){
    newConnectionCallback = _cb;
}