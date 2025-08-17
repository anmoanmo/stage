/*实现异步工作器*/
#pragma once

#include "buffer.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace mylog
{
    using Functor = std::function<void(Buffer &buffer)>;

    class AsyncLooper
    {
    public:
        using ptr = std::shared_ptr<AsyncLooper>;
        AsyncLooper(const Functor &callback)
            : _running(true),
              _callBack(callback),
              _thread(&AsyncLooper::threadEntry, this) {}

        ~AsyncLooper()
        {
            stop();
        }

        void kick()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _cond_con.notify_one();
        }

        void stop()
        {
            // 解决：二次join造成导致 std::terminate
            bool expected = true;
            if (!_running.compare_exchange_strong(expected, false))
            {
                // 已经停止过了，直接返回
                return;
            }
            {
                // 如果 push() 正在 wait，stop() 只唤醒了 _cond_con，生产者可能仍卡在 _cond_pro 上。
                std::lock_guard<std::mutex> lk(_mutex);
                _cond_pro.notify_all(); // 也唤醒生产者
                _cond_con.notify_all(); // 唤醒消费者
            }
            if (_thread.joinable())
                _thread.join();
        }

        void push(const char *data, size_t len)
        {
            // 既支持扩容，也在空间不足时能阻塞等待；stop() 会 notify_all 让这里退出。
            std::unique_lock<std::mutex> lock(_mutex);
            while (_running)
            {
                if (_pro_buf.push(data, len))
                {                           // 先尝试扩容+写入
                    _cond_con.notify_one(); // 通知消费者有数据
                    return;
                }
                _cond_pro.wait(lock); // 仍然写不进去就等消费者释放
            }
        }

        void setMaxBufferSize(size_t max_size)
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _pro_buf.resize(max_size); // 调整上限（不强行收缩当前容量）
            _con_buf.resize(max_size);
        }

        // void setAsyncBufferGrowth(size_t threshold, size_t increment)
        // {
        //     _async_threshold = threshold;
        //     _async_increment = increment;
        // }

    private:
        // 工作线程：对消费缓冲区中的数据进行处理，处理完毕后初始化缓冲区，交换缓冲区
        void threadEntry()
        {
            while (1)
            {
                {
                    // 为互斥锁形成生命周期，交换后lock解锁
                    // 1.判断生产缓冲区有没有数据，有则交换，无则阻塞
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 若当前缓冲区有数据或者running为真继续向下运行；反之阻塞休眠
                    _cond_con.wait(lock, [&]
                                   { return !_pro_buf.empty() || !_running; });
                    //运行已结束且生产缓冲区已无数据才可推出（否则可能导致缓冲区数据未写完就退出）
                    if (!_running && _pro_buf.empty())
                    {
                        break;
                    }
                    _con_buf.swap(_pro_buf);
                    // 4.唤醒生产者
                    _cond_pro.notify_all();
                }

                // 2.被唤醒后，对消费缓冲区的数据进行处理
                if (_callBack)
                {
                    _callBack(_con_buf);
                };
                // 3.初始化消费缓冲区
                _con_buf.reset();
            }
        };
        Functor _callBack; // 由异步工作器的使用者传入对应buffer

    private:
        std::atomic<bool> _running;        // 工作标志
        Buffer _pro_buf;                   // 生产缓冲区
        Buffer _con_buf;                   // 消费缓冲区
        std::mutex _mutex;                 // 互斥锁
        std::condition_variable _cond_pro; // 生产者条件变量
        std::condition_variable _cond_con; // 消费者条件变量
        std::thread _thread;               // 异步工作器对应的工作线程
    };
}