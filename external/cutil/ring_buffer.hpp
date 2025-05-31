#if !defined(RINGBUFFER_HPP)
#define RINGBUFFER_HPP

#include "basics.hpp"
#include <array>

// A thread-safe ring buffer (circular buffer)
template<typename Type,uint32_t CAP>
class ring_buffer {
public:
    ring_buffer()
        : buffer_(), head_(0), tail_(0), size_(0) {}
    ~ring_buffer(){
        while(size_ > 0){
            buffer_.data[tail_].~Type();
            tail_ = (tail_ + 1) % CAP;
            --size_;
        }
    }

    void clean(){
        while(size_ > 0){
            buffer_.data[tail_].~Type();
            tail_ = (tail_ + 1) % CAP;
            --size_;
        }
        tail_ = head_ = 0;
    }

    // Add an element to the buffer (blocking if full)
    void push(const T& item) {
        if(size_ >= CAP)
            throw std::out_of_range("push(): ring is full");

        new (&buffer_.data[head_]) Type(item);
        head_ = (head_ + 1) % CAP;
        ++size_;

        lock.unlock();
        not_empty_cv_.notify_one();
    }

    /// @brief get a copy of back
    /// @return using copy constructor!
    Type get_back(){
        if(size_ <= 0)
            throw std::out_of_range("get_back(): empty ring");

        Type item = buffer_.data[tail_];

        lock.unlock();

        return item;
    }

    // Non-blocking try-pop (returns optional)
    void try_pop() {
        if (size_ == 0) {
            return;
        }

        buffer_.data[tail_].~Type();
        tail_ = (tail_ + 1) % CAP;
        --size_;

        not_full_cv_.notify_one();
    }

    bool empty(){
        if (size_ == 0)
            return true;
        return false;
    }
    bool full(){
        if (size_ >= CAP)
            return true;
        return false;
    }

private:
    uint32_t head_;
    uint32_t tail_;
    uint32_t size_;
    union
    {
        std::array<Type,CAP> data;
        bool _;
        buffer():_{false}{}
    } buffer_;
};

#endif // RINGBUFFER_HPP
