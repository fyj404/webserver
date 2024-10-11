#pragma once
#include <sys/epoll.h>
#include <vector>
//前向声明
class Channel;
// 前向声明（Forward Declaration）是一种在C或C++编程中使用的声明方式，用于告诉编译器某个类型的存在，而无需提供其完整定义。它通常用于解决类型依赖的问题，尤其是在类或结构体之间相互引用的情况下。

// 前向声明的用途
// 避免循环依赖：

// 当两个类相互引用时，如果没有前向声明，编译器可能无法确定它们的完整定义，导致编译错误。前向声明可以解决这一问题。
// 例如，如果 ClassA 中包含对 ClassB 的指针，而 ClassB 中又包含对 ClassA 的指针，可以使用前向声明来打破这种循环依赖。
// 减少包含文件的数量：

// 使用前向声明可以减少头文件之间的依赖关系，从而减少不必要的包含文件。这样可以加快编译速度。
class Epoll
{
private:
    int epfd;
    struct epoll_event *events;
public:
    Epoll();
    ~Epoll();

    void addFd(const int fd, const uint32_t op);
    void updateChannel(Channel*);
    // std::vector<epoll_event> poll(const int timeout = -1);
    std::vector<Channel*> poll(int timeout = -1);
};