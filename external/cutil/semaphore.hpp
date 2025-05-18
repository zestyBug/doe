#ifndef _SEMAPHORE_HPP
#define _SEMAPHORE_HPP  1

#include <mutex>
#include <condition_variable>

class Semaphore {
    std::mutex mutex_;
    std::condition_variable condition_;
    volatile unsigned long count_ = 0; // Initialized as locked.

public:
    Semaphore(unsigned long initial_value = 0) : count_(initial_value) {}
    // signal
    void release() {
        std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
        ++this->count_;
        this->condition_.notify_one();
    }
    // wait
    void acquire() {
        std::unique_lock<decltype(this->mutex_)> lock(this->mutex_);
        while (count_ == 0) // Handle spurious wake-ups.
            this->condition_.wait(lock);
        --this->count_;
    }
    // try wait()
    bool tryAcquire() {
        std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
        if (this->count_ != 0) {
            --this->count_;
            return true;
        }
        return false;
    }
    void setValue(const unsigned long value){
            std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
        this->count_ = value;
    }
};

#endif