#if !defined(APPENDBUFFER_HPP)
#define APPENDBUFFER_HPP

#include "basics.hpp"

class append_buffer
{
    align_ptr<uint8_t[]> _buffer;
    uint32_t _size = 0;
    uint32_t _capacity = 0;
    static const uint32_t minimum_capacity = 64;
    void extend(){
        const uint32_t new_cap = _capacity * 2;
        align_ptr<uint8_t[]> temp = make_align<uint8_t[]>(new_cap);
        memcpy(temp.get(),this->_buffer.get(),this->_size);
        this->_buffer = std::move(temp);
        this->_capacity = new_cap;
    }
    public:
    inline uint32_t length() const {
        return this->_size;
    }
    inline void * ptr() {
        return this->_buffer.get();
    }
    append_buffer(uint32_t initial_capacity = 1024):_size{0},_capacity{initial_capacity}{
        if(initial_capacity < minimum_capacity)
            initial_capacity = minimum_capacity;
        _buffer = make_align<uint8_t[]>(initial_capacity);
    }
    template<typename T>
    void add(T v){
        if(_capacity < (sizeof(T)+_size))
            extend();
        void *ptr = _buffer.get() + _size;
        memcpy(ptr,&v,sizeof(T));
        _size += sizeof(T);
    }
    inline void reset(){
        this->_size = 0;
    }
};

#endif // APPENDBUFFER_HPP
