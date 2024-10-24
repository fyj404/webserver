#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
    /*

    信号量是一种用于控制对共享资源访问的同步机制，广泛应用于多线程或多进程环境中。
    它可以帮助协调多个线程之间的操作，确保资源的安全使用。信号量有两种主要类型：

    1. 二元信号量（Binary Semaphore）
    只允许一个线程访问共享资源，类似于互斥锁。
    通常用于保护临界区，确保在同一时刻只有一个线程可以执行特定代码。
    2. 计数信号量（Counting Semaphore）
    允许多个线程访问共享资源，具体数量由信号量的值决定。
    值的初始设置可以表示可用资源的数量，线程在获取资源时减少信号量的值，释放资源时增加信号量的值。
    
    */

   /*
   sem_init(&semId_, 0, MAX_CONN_);: 初始化信号量，设定其初始值为最大连接数。
   这意味着最多可以同时有 MAX_CONN_ 个线程获取连接。
   sem_wait(&semId_);: 在获取连接时，调用 sem_wait 会检查信号量的值。
   如果有可用的连接，信号量减1，允许线程继续执行；如果没有可用连接，线程会被阻塞，直到其他线程释放连接。
   通过这种方式，信号量确保在任何时刻，连接池中的连接数量不会超过预设的最大值，从而避免数据库过载和资源浪费。
   调用 sem_post 增加信号量的值，表示有一个可用的连接可供其他线程使用。这将唤醒一个等待获取连接的线程（如果有的话）。
   */
};


#endif // SQLCONNPOOL_H