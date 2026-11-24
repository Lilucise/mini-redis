#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <iostream>

class ThreadPool{
public:
    explicit ThreadPool(size_t num_threads);        // 构造
    ~ThreadPool();                                  // 析构

    void enqueue(std::function<void()> task);       // 任务入队
    
private:
    std::vector<std::thread> workers_;              // 工作线程
    std::queue<std::function<void()>> tasks_;       // 任务队列
    std::mutex mtx_;                                // 保护tasks_和stop_
    std::condition_variable cv_;                    // 有任务时唤醒线程
    bool stop_ = false;                             // 停止标志位
};

ThreadPool::ThreadPool(size_t num_threads){
    for(size_t i = 0; i < num_threads; i++){
        // 线程绑定一个死循环任务
        workers_.emplace_back(
            [this]{
                while(true){
                    std::unique_lock<std::mutex> lock(mtx_);    // 加锁
                    cv_.wait(lock, [this]{ return stop_ || !tasks_.empty();});   // 有任务或停止就不等待

                    if(stop_ && tasks_.empty()) break;  // 任务处理完且需要停止就跳出循环(结束)

                    // 处理队首任务
                    auto task = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();  // 解锁

                    task(); // 执行
                }
            }
        );
    }
}

ThreadPool::~ThreadPool(){
    std::unique_lock<std::mutex> lock(mtx_);    // 加锁
    stop_ = true;
    lock.unlock();

    cv_.notify_all();
    for(auto& worker : workers_){
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task){
    {
        std::lock_guard<std::mutex> guard(mtx_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

#endif