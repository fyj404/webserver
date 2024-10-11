#pragma once

class InetAddress;
class Socket
{
private:
    int fd;
public:
    Socket();
    Socket(int);
    ~Socket();

    void bind(const InetAddress*)const;
    void listen()const;
    void setnonblocking();

    int accept(InetAddress*);

    int getFd()const;
};