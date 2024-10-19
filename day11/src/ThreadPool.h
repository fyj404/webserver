#pragma once
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class ThreadPool
{
private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mtx;
    std::condition_variable cv;
    bool stop;
public:
    ThreadPool(int size = 10);
    ~ThreadPool();

    // void add(std::function<void()>);
    template<class F, class... Args>
    auto add(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>;

};


//不能放在cpp文件，原因是C++编译器不支持模版的分离编译
//函数接受一个可调用对象 F 和任意数量的参数 Args。这使得它可以接收任何类型的任务和参数。
//std::result_of 获取调用对象 F 使用参数 Args... 后的返回类型，并命名为 return_type。
//auto：表示返回类型使用了后置返回类型语法，实际返回类型在 -> 后面定义。
//F&& f：这是一个模板参数，F 表示一个可调用对象
//（如函数、函数对象或 lambda 表达式）
//&& 是右值引用修饰符，这意味着该函数可以接受一个右值或左值引用，允许移动语义和完美转发。
//Args&&... args：这是一个可变参数模板，表示可以接受任意数量的参数，
//Args 是参数类型的模板，&& 也允许这些参数的左值或右值引用。... 表示这是一个参数包，可以接收多个参数。
//std::future<...>：std::future 是 C++ 标准库中的一个模板类，
//用于表示异步操作的结果。调用者可以使用这个对象在未来获取任务的结果。
//typename std::result_of<F(Args...)>::type：std::result_of 是一个模板，
//用于获取给定可调用对象（在这里是 F）使用特定参数类型（在这里是 Args...）时的返回类型。
template<class F, class... Args>
auto ThreadPool::add(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    //通过 std::packaged_task 封装实际的任务，任务内容是将 f 和 args 绑定在一起。std::bind 生成一个可调用对象。
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(tasks_mtx);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        //tasks.emplace(...)：将一个新的任务添加到 tasks 队列中。
        //[task](){ (*task)(); }：这是一个 Lambda 表达式，捕获 task 指针，
        //并在任务执行时调用 (*task)()，即执行 packaged_task 中封装的函数。
        tasks.emplace([task](){ (*task)(); });
    }
    cv.notify_one();
    return res;
}