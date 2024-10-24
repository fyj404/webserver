
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <semaphore.h>
class ThreadPool {

public:
    //explicit: 防止隐式转换，确保只有在需要时显式创建 ThreadPool 对象。
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            //assert(threadCount > 0);
            for(size_t i = 0; i < threadCount; i++) {

                //std::thread([pool = pool_] {...}): 使用 Lambda 表达式定义线程的执行逻辑，
                //同时捕获共享指针 pool_，确保每个线程都可以访问线程池。
                std::thread([pool = pool_] {
                    //std::unique_lock<std::mutex> locker(pool->mtx);: 
                    //创建一个独占锁 locker，用于保护任务队列的并发访问。
                    std::unique_lock<std::mutex> locker(pool->mtx);

                    while(true) {
                        if(!pool->tasks.empty()) {
                            // 获取并移动队列中的第一个任务。
                            auto task = std::move(pool->tasks.front());
                            pool->tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } 
                        else if(pool->isClosed) break;//如果线程池已关闭，退出循环，终止线程。
                        else pool->cond.wait(locker);//如果任务队列为空且线程池未关闭，当前线程会在条件变量 cond 上等待，
                        //释放锁并挂起，直到被其他线程通知有新任务可执行
                    }
                }).detach();//将创建的线程分离，使其在后台运行，与主线程独立，避免主线程和子线程之间的资源管理问题。
            }
    }
    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        //if(static_cast<bool>(pool_)): 检查 pool_ 是否有效，即是否指向一个有效的 Pool 对象。
        //使用 static_cast<bool> 是为了确保进行类型安全的布尔转换。
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

#endif