#include "httpconn.h"
using namespace std;

//srcDir 存储静态资源的目录。
const char* HttpConn::srcDir;
//userCount 用于统计当前连接的用户数量，使用原子变量以确保线程安全。
std::atomic<int> HttpConn::userCount;
//isET 是一个布尔值，指示是否使用边缘触发（Edge Triggered）模式。
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};

//初始化连接，设置文件描述符和地址，并清空读取和写入缓冲区。
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    //增加连接用户计数，并记录日志。
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}


//从连接读取数据，使用边缘触发模式。如果读取失败，返回相应的错误码。
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    request_.Init();
    //检查读取缓冲区 readBuff_ 中的可读字节数。如果没有可读字节，则返回 false，表示请求未能处理。
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {
        //初始化响应对象 response_，传入源目录、请求路径、保持连接状态（Keep-Alive）和状态码 200（表示成功）
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        //如果请求解析失败，初始化响应对象 response_，设置状态码为 400（表示错误请求）
        response_.Init(srcDir, request_.path(), false, 400);
    }
    //调用 MakeResponse 方法生成响应，并将结果写入 writeBuff_ 中
    response_.MakeResponse(writeBuff_);
    /* 响应头 */
    //iov_[0] 是第一个 iovec 结构，指向写入缓冲区的可读部分。
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    //将 iovCnt_ 设置为 1，表示当前只准备一个 iovec。
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) {
        //检查响应是否包含文件（文件长度大于 0 且文件指针有效）。如果有文件：
        //设置第二个 iovec iov_[1] 指向文件的基础地址和长度。
        //将 iovCnt_ 更新为 2，表示现在有两个 iovec。
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}