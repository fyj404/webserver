#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

#define BUFFER_SIZE 1024 

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");
    
    while(true){
        char buf[BUFFER_SIZE];  //在这个版本，buf大小必须大于或等于服务器端buf大小，不然会出错，想想为什么？
        //如果客户端的 buf 小于服务器发送的消息大小，read 调用会导致客户端读取不完整的消息，可能出现数据截断或者数据丢失的情况。
        //特别是在非阻塞I/O的情况下，小缓冲区可能导致未能完全读取服务器发送的数据，后续消息可能被忽略或错误处理。
        //个人感觉主要是server用了非阻塞,client用了阻塞,所以client要一次性把server发的都读过来
        bzero(&buf, sizeof(buf));
        scanf("%s", buf);
        //理想情况下，应该用 strlen(buf) 来发送实际的字符串长度，而不是整个缓冲区大小。
        ssize_t write_bytes = write(sockfd, buf, sizeof(buf));
        if(write_bytes == -1){
            printf("socket already disconnected, can't write any more!\n");
            break;
        }
        bzero(&buf, sizeof(buf));
        ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
        if(read_bytes > 0){
            printf("message from server: %s\n", buf);
        }else if(read_bytes == 0){
            printf("server socket disconnected!\n");
            break;
        }else if(read_bytes == -1){
            close(sockfd);
            errif(true, "socket read error");
        }
        //当 read_bytes 等于 0 时，表示服务器已经断开连接，客户端也应退出循环。
        //当 read_bytes 等于 -1 时，表示读取数据时出错。此时客户端关闭套接字并输出错误信息。
    }
    close(sockfd);
    return 0;
}