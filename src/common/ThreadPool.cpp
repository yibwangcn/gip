#include "common/ThreadPool.hpp"

namespace IP::common::ThreadPool
{
    ThreadPool::ThreadPool(size_t num_threads) : stop_(false), logger_("common::ThreadPool: ")
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers_.emplace_back(std::thread(&ThreadPool::worker, this));
        }
        TRC_INFO << logger_ << "Initializd ThreadPool with " << num_threads << " threads.";
    }

    ThreadPool::~ThreadPool()
    {
        stop();
    }

    void ThreadPool::worker()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (tasks_.empty())
            {
                condition_.wait(lock, [this]
                                { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty())
                {
                    break;
                }
            }

            auto task = std::move(tasks_.front());
            tasks_.pop();
            lock.unlock();

            try
            {
                task();
            }
            catch (const std::exception &e)
            {
                TRC_WARNING << logger_ << "Exception in thread pool task: " << e.what();
            }
            catch (...)
            {
                TRC_WARNING << logger_ << "Unknown exception in thread pool task.";
            }
        }
    }

    void ThreadPool::stop()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto &worker : workers_)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }
} // namespace IP::common::ThreadPool
