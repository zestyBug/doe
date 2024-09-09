#ifndef ADVANCEDSTACK_HPP
#define ADVANCEDSTACK_HPP 1

#include <new>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#define ARRAY_DEBUG 1

#define invalid_index -1
#ifdef ARRAY_DEBUG
#include <stdio.h>
#define ADEBUG(...) printf(__VA_ARGS__)
#else
#define ADEBUG(...) ;
#endif // ARRAY_DEBUG

/// @brief stack with big block allocation
/// @tparam T data type with none zero size
/// @tparam blocksize template because only can be copied to same class type
template<typename T,size_t blocksize>
class advancedStack {
    struct block
    {
        block *next;
        T data[0];
    };

    // there will never be an empty block
    block *tail = nullptr;
    // so it will never be 1
    size_t tail_index = 1;

    void alloc()
    {
        if(tail){
            block *new_tail = malloc(sizeof(block) + sizeof(T)*blocksize);
            new_tail->next = tail;
            tail = new_tail;
        }else{
            tail = malloc(sizeof(block) + sizeof(T)*blocksize);
            tail->next = nullptr;
        }
        tail_index=1;
    }
    void dealloc()
    {
        block *new_tail = this->tail->next;
        free(this->tail);
        this->tail = new_tail;
        this->tail_index = blocksize+1;
    }
public:
    advancedStack() = default;

    template<typename ... Arg>
    void emplace_back(Arg &&... Args)
    {
        // allocate only if needed
        if(tail_index == (blocksize+1))
            alloc();
        new (this->tail->data + (blocksize - tail_index++) ) T(Args...);
    }

    void pop_back() {
        if(!this->tail)
            return;
        this->tail->data[blocksize - --tail_index].~T();
        if(tail_index == 1)
            dealloc();
    }
};


#endif