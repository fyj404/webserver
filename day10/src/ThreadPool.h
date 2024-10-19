#pragma once
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mtx;
    std::condition_variable cv;//td::condition_variable 是 C++ 中线程间同步的工具，主要用于线程间的等待和通知机制。
    //它通常与 std::mutex 配合使用，来让一个或多个线程等待某个条件成立后继续执行，而另一个线程可以在满足条件后通知等待的线程继续执行
    bool stop;
public:
    ThreadPool(int size = 10);
    ~ThreadPool();

    void add(std::function<void()>);

};