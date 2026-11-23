#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <function>
#include <vector>

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

