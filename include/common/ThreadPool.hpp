#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "Logger.hpp"

namespace IP::common::ThreadPool
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
        ~ThreadPool();

        void stop();

        template <typename Func, typename... Args>
        bool enqueue(Func &&func, Args &&...args)
        {
            auto task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            return enqueue(task);
        }

        template <typename Func>
        bool enqueue(Func &&func)
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_)
            {
                TRC_WARNING << logger_<<"Attempt to enqueue task on stopped ThreadPool.";
                return false;
            }
            tasks_.emplace(std::forward<Func>(func));
            condition_.notify_one();
            return true;
        }

    private:
        void worker();

        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        bool stop_;
        std::string logger_;
    };

} // namespace IP::common::ThreadPool
#endif /* __THREAD_POOL_HPP__ */
