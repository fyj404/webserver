#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);
    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size()const;
    size_t capacity()const;

    T front()const;

    T back()const;
    void push_back(const T &item);

    void push_front(const T &item);

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();
private:
    std::deque<T> deq_;
    size_t capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);//用于在调试模式下验证条件 MaxCapacity > 0 是否成立。
    //如果该条件为假（即 MaxCapacity 小于或等于 0），程序会触发断言失败，通常会终止执行，并输出错误信息。
    //在非调试模式下，assert 语句通常会被编译器忽略，不会对程序的执行产生影响。
    //这意味着，如果程序在发布版本中运行，assert(MaxCapacity > 0); 不会被检查，程序将继续执行而不进行此条件的验证。
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
};

template<class T>
void BlockDeque<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};

template<class T>
void BlockDeque<T>::flush() {
    condConsumer_.notify_one();
};

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
T BlockDeque<T>::front() const{
    std::lock_guard<std::mutex> locker(mtx_);
    if (deq_.empty()) {
        throw std::out_of_range("Deque is empty");
    }
    return deq_.front();
}

template<class T>
T BlockDeque<T>::back() const{
    std::lock_guard<std::mutex> locker(mtx_);
    if (deq_.empty()) {
        throw std::out_of_range("Deque is empty");
    }
    return deq_.back();
}

template<class T>
size_t BlockDeque<T>::size() const{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
size_t BlockDeque<T>::capacity() const{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    //使用 std::unique_lock<std::mutex> 创建一个锁 locker，用于保护对 deq_ 的访问。此锁在其作用域内有效，自动释放
    std::unique_lock<std::mutex> locker(mtx_);
    //使用 while(deq_.size() >= capacity_) 检查队列是否已满。如果队列的大小大于或等于 capacity_，则表示无法添加更多元素。
    while(deq_.size() >= capacity_) {
        //condProducer_.wait(locker) 将当前线程放入等待状态，
        //并释放锁，直到有其他线程通过条件变量（condConsumer_）通知它。这样可以避免活锁，确保资源有效利用。
        condProducer_.wait(locker);
    }
    //一旦队列有空间，调用 deq_.push_back(item) 将元素添加到队列末尾。
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        condConsumer_.wait(locker);// 进入等待状态，释放锁并等待条件变量
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();//// 唤醒一个等待的生产者线程
    return true;
}

//这段代码定义了 BlockDeque 类的 pop 方法，增加了一个超时参数，用于从双端队列中移除并返回前端元素
template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

#endif // BLOCKQUEUE_H