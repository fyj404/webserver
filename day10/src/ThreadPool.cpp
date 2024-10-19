#include "ThreadPool.h"

ThreadPool::ThreadPool(int size) : stop(false){

    //循环创建指定数量的线程，并将其添加到 threads 容器中。每个线程执行一个 lambda 函数。
    for(int i = 0; i < size; ++i){
        threads.emplace_back(std::thread([this](){
            while(true){
                
                std::function<void()> task;
                {
                    //使用 std::unique_lock<std::mutex> lock(tasks_mtx) 进行加锁，保护对任务队列 (tasks) 的访问。
                    std::unique_lock<std::mutex> lock(tasks_mtx);

                    //cv.wait(lock, [this]() { return stop || !tasks.empty(); });：
                    //线程等待条件变量 cv，直到 stop 为 true 或者 tasks 队列不为空
                    cv.wait(lock, [this](){
                        return stop || !tasks.empty();
                    });
                    if(stop && tasks.empty()) return;
                    task = tasks.front();
                    tasks.pop();
                }
                task();
            }
        }));
    }
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(tasks_mtx);
        stop = true;//通过加锁设置 stop 为 true，表示线程池要停止。
    }
    cv.notify_all();//调用 cv.notify_all() 唤醒所有正在等待的线程
    for(std::thread &th : threads){
        if(th.joinable())
            th.join();
    }//遍历所有线程，如果线程可加入（即仍在运行），调用 th.join() 等待它们完成。
}

void ThreadPool::add(std::function<void()> func){
    {
        std::unique_lock<std::mutex> lock(tasks_mtx);
        if(stop)
            throw std::runtime_error("ThreadPool already stop, can't add task any more");
        tasks.emplace(func);//将任务添加到任务队列中 tasks.emplace(func);
    }
    cv.notify_one();//调用 cv.notify_one()，唤醒一个等待线程，通知它有新任务可处理
}