#include "heaptimer.h"

//这段代码是一个实现二叉堆中“上浮”操作的函数，通常用于维护堆的性质。
//在二叉堆中，每个节点的值都必须小于或等于其子节点的值（对于最小堆）或大于或等于其子节点的值
void HeapTimer::siftup_(size_t i) {
    //这行代码用于检查索引 i 是否有效，即 i 是否在堆的范围内。heap_ 是存储堆元素的容器
    assert(i >= 0 && i < heap_.size());
    //计算节点 i 的父节点索引 j。在完全二叉树中，父节点的索引可以通过 (子节点索引 - 1) / 2 得到。
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        //如果父节点的值小于等于当前节点的值，则堆的性质已经满足，跳出循环。
        if(heap_[j] < heap_[i]) { break; }
        //交换
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

//这段代码是一个实现二叉堆中“下沉”操作的函数，
//主要用于在删除堆顶元素或调整堆结构时维护堆的性质。它确保子节点的值不会小于（对于最大堆）或大于（对于最小堆）父节点的值。
bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    //将当前节点的索引初始化为 index。
    size_t i = index;
    //计算当前节点的左子节点的索引 j。在完全二叉树中，左子节点的索引为 2 * 父节点索引 + 1。(从0开始的话是左儿子,从1开始是右儿子)
    size_t j = i * 2 + 1;

    while(j < n) {
        //如果右儿子小,那就更新为右
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        //堆的性质已满足，跳出循环。
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}

void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::del_(size_t index) {
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
    siftdown_(ref_[id], heap_.size());
}

void HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}