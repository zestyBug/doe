#ifndef ADVANCED_QUEUE_HPP
#define ADVANCED_QUEUE_HPP
#include <new>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/// @brief Advanced queue
/// @tparam T data type
/// @tparam BS block size
template<typename T,size_t BS>
class advancedQueue final {
public:
    union block
    {
        T value[BS];
        bool _;
        block():_{false}{}
    };
    struct blockStructure
    {
        block data;
        blockStructure *next;
    };
    struct index {
        size_t index = 0;
        blockStructure *block = nullptr;
    };
    // writes to head
    index head;
    // read from tail
    index tail;


    advancedQueue()
    {
        this->tail.block = this->head.block = malloc(sizeof(blockStructure));
        this->head.block->next = nullptr;
    }

    ~advancedQueue()
    {
        //
    }

    T* back()
    {
        if(this->head.block == this->tail.block)
            if(this->tail.index >= this->head.index)
                return nullptr;
        return this->tail.block + this->tail.index;
    }
    void pop_back(){
        if(this->head.block == this->tail.block){
            if(this->tail.index >= this->head.index){
                return;
            }else{
                this->tail.block->data.value[this->tail.index].~T();
                this->tail.index++;
            }
        }else{
            this->tail.block->data.value[this->tail.index].~T();
            this->tail.index++;
            if(this->tail.index >= BS){
                void * const cache = this->tail.block;
                this->tail.block = this->tail.block->next;
                free(cache);
            }
        }
    }
};

#endif // ADVANCED_QUEUE_HPP
