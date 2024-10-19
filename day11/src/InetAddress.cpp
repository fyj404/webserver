#include "InetAddress.h"
#include <string.h>
//addr_len：这是一个类成员变量，用于存储 addr 的长度。
//addr 是一个 sockaddr_in 结构体，用于存储 IPv4 地址和端口信息。
//bzero(&addr, sizeof(addr))：这行代码将 addr 结构体的所有字节设置为 0，以确保它处于一个已知的初始状态。
//#include <string.h>：引入字符串处理函数，如 bzero。在 C++ 中，通常更推荐使用 std::memset 来替代 bzero。
InetAddress::InetAddress() : addr_len(sizeof(addr)){
    bzero(&addr, sizeof(addr));
}
//这个构造函数接受两个参数：一个字符串表示的 IP 地址和一个端口号。

InetAddress::InetAddress(const char* ip, const uint16_t port) : addr_len(sizeof(addr)){
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET; //设置地址族为 IPv4。
    addr.sin_addr.s_addr = inet_addr(ip);//调用 inet_addr 函数将传入的 IP 地址字符串转换为网络字节序的二进制格式，并存储在 addr.sin_addr.s_addr 中。
    addr.sin_port = htons(port);//调用 htons 函数将传入的端口号从主机字节序转换为网络字节序，并存储在 addr.sin_port 中。
    addr_len = sizeof(addr);//最后，addr_len 被设置为 addr 的大小。
}

InetAddress::~InetAddress(){
}
void InetAddress::setInetAddr(sockaddr_in _addr, socklen_t _addr_len){
    addr = _addr;
    addr_len = _addr_len;
}
sockaddr_in InetAddress::getAddr()const{
    return addr;
}
socklen_t InetAddress::getAddr_len()const{
    return addr_len;
}