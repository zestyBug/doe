#if !defined(QueeuQueue_hpp)
#define QueeuQueue_hpp

#include <stdint.h>
#include <iostream>

template<typename Type, size_t BS>
class QueueQueue {
    static constexpr size_t block_size = BS;
    struct blockStructure{Type value[block_size];blockStructure *next = nullptr;};
    struct index {size_t index = 0;blockStructure *block = nullptr;};
#if !defined(__cpp_lib_hardware_interference_size)
    #define  __cpp_lib_hardware_interference_size 64
#endif
    // writes to head
    alignas(__cpp_lib_hardware_interference_size) volatile index head;
    // read from tail
    alignas(__cpp_lib_hardware_interference_size) volatile index tail;
public:
    QueueQueue(){
        this->tail.block = this->head.block = new blockStructure();
    }
    ~QueueQueue(){
        while (this->tail.block)
        {
            blockStructure *next = this->tail.block->next;
            delete this->tail.block;
            this->tail.block = next;
        }
        this->tail.block = this->head.block = nullptr;
    }
    void push(const Type &val){
        // next block is allocatted if only needed
        if (this->head.index >= this->block_size){
            blockStructure *blk = new blockStructure();
            blk->value[0] = val;
            this->head.block = (this->head.block->next = blk);
            this->head.index = 1;
        }else{
            this->head.block->value[this->head.index] = val;
            // its important to increase index only after data is burn on memory
            // so readers dont early access to unavailable data
            this->head.index++;
        }
    }
    // warn: dont write to this instance and dont read from Q while appening
    void append(cosnt QueueQueue<Type,BS> &Q){
        blockStructure *t_block = Q.tail.block;
        size_t t_index = Q.tail.index;

        while(true)
        {
            if(t_index <  Q.head.index) goto cnt;
            if(t_block != Q.head.block) goto cnt;
            break;
        cnt:
            if(t_index >= Q.block_size)
            {
                t_block = t_block->next;
                t_index = 0;
            }
            this->push(t_block->value[t_index++]);
        }
    }
    // it is thread safe on single thread access read.
    bool empty() const {
    //if(this->tail.block != this->head.block || 
    //   this->tail.block == this->head.block){
        if(this->tail.index < this->head.index)
            return false;
        //else if(this->tail.index >= this->head.index){
            if(this->tail.block != this->head.block)
                return false;
            //else if(this->tail.block == this->head.block)
                return true;
    //}
    }
    Type return_and_pop(){
        if(this->empty())
            // return Type{} or throw an exception!
            throw std::exception();

        // if reached end of block
        if(this->tail.index >= this->block_size)
        {
            // technically next block must be neither null nor empty at this point
            blockStructure *next = this->tail.block->next;
            delete this->tail.block;
            this->tail.index = 0;
            this->tail.block = next;
        }
        return std::move(this->tail.block->value[this->tail.index++]);
    }
};

#endif // QueeuQueue_hpp
