#ifndef _SEMAPHORE_HPP
#define _SEMAPHORE_HPP  1

#include <mutex>
#include <condition_variable>

/// @brief A mutex for recource
class Semaphore {
    std::mutex mutex_;
    std::condition_variable condition_;
    volatile uint32_t count_ = 0; // Initialized as locked.

public:
    Semaphore(unsigned long initial_value = 0) : count_(initial_value) {}
    /// @brief signal semaphore is available.
    void release() {
        std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
        ++this->count_;
        this->condition_.notify_one();
    }
    /// @brief use it with caution
    void release(uint32_t num) {
        std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
        this->count_ += num;
        this->condition_.notify_all();
    }
    /// @brief wait semaphore the resource to be available
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