#include "sqlconnpool.h"
using namespace std;

//useCount_: 这个成员变量用于跟踪当前正在使用的连接数量，初始值为 0。
//freeCount_: 这个成员变量用于跟踪当前可用的连接数量，初始值为 0。当连接被创建并未被使用时，该计数器将增加。
SqlConnPool::SqlConnPool() {
    useCount_ = 0;
    freeCount_ = 0;
}
//这里是用指针实现单例模型
//在log.h的Log类是用的引用实现单例模式
//使用指针的话 可以明确表示返回值可能为 nullptr，使得调用者可以检查是否成功获取到实例
//引用不能被重新赋值，固定引用指向某个特定实例。
//其他的也没啥区别了
SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int connSize = 10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        //使用 MySQL C API 的 mysql_init 函数初始化一个新的连接句柄。传入的 sql 为 nullptr，因此会返回一个新的 MYSQL 结构
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        //这里加个else会不会更好
        connQue_.push(sql);
    }
    // 保存最大连接数，用于后续管理
    MAX_CONN_ = connSize;
    //sem_init(&semId_, 0, MAX_CONN_);: 
    //初始化信号量 semId_，以控制连接的并发使用，初始值为最大连接数 MAX_CONN_
    sem_init(&semId_, 0, MAX_CONN_);
}

//GetConn 方法用于从连接池获取一个可用的数据库连接。
MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;

    //if(connQue_.empty()): 检查连接队列是否为空。
    //如果为空，记录警告信息并返回 nullptr，表示连接池忙碌或没有可用连接。
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    //sem_wait(&semId_);: 等待信号量，确保有可用的连接。这个操作会阻塞当前线程，直到有可用连接。
    sem_wait(&semId_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(sql);
    //调用 sem_post 增加信号量的值，表示有一个可用的连接可供其他线程使用。这将唤醒一个等待获取连接的线程（如果有的话）。
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    //mysql_library_end();: 结束 MySQL 库的使用，清理相关资源。这通常在所有 MySQL 操作完成后调用。
    mysql_library_end();  

}

int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}