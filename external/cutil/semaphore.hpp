#ifndef _SEMAPHORE_HPP
#define _SEMAPHORE_HPP  1

#include <mutex>
#include <condition_variable>

class semaphore {
    std::mutex mutex_;
    std::condition_variable condition_;
    unsigned long count_ = 0; // Initialized as locked.

public:
    semaphore(unsigned long initial_value = 0) : count_(initial_value) {}
    // signal
    void release() {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        ++count_;
        condition_.notify_one();
    }
    // wait
    void acquire() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        while (!count_) // Handle spurious wake-ups.
            condition_.wait(lock);
        --count_;
    }
    // try wait()
    bool try_acquire() {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (count_) {
            --count_;
            return true;
        }
        return false;
    }
};

#endif