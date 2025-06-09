#if !defined(RINGBUFFER_HPP)
#define RINGBUFFER_HPP

#include "basics.hpp"
#include <atomic>
#include <vector>
#include <optional>

template<typename T>
class mrsw_ring_buffer {
public:
    mrsw_ring_buffer(uint32_t _SIZE) : write_index(0), read_index(0), SIZE{_SIZE} {
        buffer.resize(_SIZE);
    }

    // Writer-only thread calls this
    bool push(const T& item) {
        uint32_t w = write_index.load(std::memory_order_relaxed);
        uint32_t r = read_index.load(std::memory_order_acquire);

        if ((w - r) >= SIZE)
            return false; // buffer full

        buffer[w % SIZE] = item;
        write_index.store(w + 1, std::memory_order_release);
        return true;
    }

    // Reader threads call this
    std::optional<T> pop() {
        uint32_t r;
        do {
            r = read_index.load(std::memory_order_acquire);
            uint32_t w = write_index.load(std::memory_order_acquire);
            if (r == w) {
                return std::nullopt; // buffer empty
            }
        } while (!read_index.compare_exchange_weak(r, r + 1, std::memory_order_acq_rel));

        return buffer[r % SIZE];
    }

private:
    std::vector<T> buffer;
    const uint32_t SIZE;
    std::atomic<uint32_t> write_index;
    std::atomic<uint32_t> read_index;
};


template<typename T>
class mrmw_buffer
{
public:
    mrmw_buffer():
        size{0},
        buffer{nullptr},
        write_index(0),
        read_index(0)
        {}
    void init(int32_t _SIZE){
        size=_SIZE;
        buffer=allocator<T>().allocate(_SIZE);
    }
    ~mrmw_buffer(){
        allocator<T>().deallocate(buffer);
    }

    bool push(const T& item) noexcept {
        int32_t w = write_index.load(std::memory_order_relaxed);
        int32_t r = read_index.load(std::memory_order_acquire);

        if (w >= buffer.size())
            return false; // buffer full

        new (buffer + w) T(item);
        write_index.store(w + 1, std::memory_order_release);
        return true;
    }

    bool push(T&& item) noexcept {
        int32_t w = write_index.load(std::memory_order_relaxed);
        int32_t r = read_index.load(std::memory_order_acquire);

        if (w >= buffer.size())
            return false; // buffer full

        new (buffer + w) T(std::move(item));
        write_index.store(w + 1, std::memory_order_release);
        return true;
    }

    T* pop() noexcept {
        int32_t r;
        do {
            r = read_index.load(std::memory_order_acquire);
            const int32_t w = write_index.load(std::memory_order_acquire);
            if (r >= w) {
                return nullptr; // buffer empty
            }
        } while (!read_index.compare_exchange_weak(r, r + 1, std::memory_order_acq_rel));
        return buffer+r;
    }

private:
    T* buffer;
    uint32_t size;
    std::atomic<uint32_t> write_index;
    std::atomic<uint32_t> read_index;
};

#endif // RINGBUFFER_HPP
