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
    InetAddress *clnt_addr = new InetAddress();      
    Socket *clnt_sock = new Socket(sock->accept(clnt_addr));      
    printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->getFd(), inet_ntoa(clnt_addr->getAddr().sin_addr), ntohs(clnt_addr->getAddr().sin_port));
    clnt_sock->setnonblocking();
    newConnectionCallback(clnt_sock);
    delete clnt_addr;
}

void Acceptor::setNewConnectionCallback(std::function<void(Socket*)> _cb){
    newConnectionCallback = _cb;
}