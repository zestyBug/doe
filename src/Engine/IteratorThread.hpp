#if !defined(ITERATORTHREAD_HPP)
#define ITERATORTHREAD_HPP

#include <thread>
#include <mutex>
#include <condition_variable>

namespace DOTS
{
    

template<typename T1,typename T2>
class IteratorThread{
    static constexpr size_t block_size = 64;
    std::thread th;
    std::mutex mutex_input;
    std::mutex mutex_output;
    std::condition_variable condition_;

    struct blockStructure1 {T1 value[block_size];blockStructure1 *next = nullptr;};
    struct index1 {size_t index = 0;blockStructure1 *block = nullptr;};

    struct blockStructure2{T2 value[block_size];blockStructure2 *next = nullptr;};
    struct index2 {size_t index = 0;blockStructure2 *block = nullptr;};

    // writes to head
    volatile index1 input_head;
    // read from tail
    index1 input_tail;

    // writes to head
    index2 output_head;
    // read from tail
    volatile index2 output_tail;

    typedef T2 (*func_t)(const T1&);
    func_t function;
    void process(){while(true){
        T1 *val;
        // input_tail must be accessed only in this block
        {
            if(this->input_tail.block == nullptr)
                return;
            std::unique_lock<decltype(this->mutex_input)> lock(this->mutex_input);
            if(this->input_tail.block == this->input_head.block
            && this->input_tail.index >= this->input_head.index)
                // only wakes up under 3 condition: 
                // 1-head moved to new index, 
                // 2-new block with atleast single value is available
                // 3-kill signal is sent
                this->condition_.wait(lock);
                
            if(this->input_tail.block == nullptr)
                return;

            // if reached end of block
            if(this->input_tail.index >= this->block_size){
                blockStructure1 *next = this->input_tail.block->next;
                delete this->input_tail.block;
                this->input_tail = index1{0,next};
            }
            val = this->input_tail.block->value + this->input_tail.index;
            this->input_tail.index++;
            // check if entity exists here
            if(false){
                continue;
            }
        }
        T2 res = this->function(*val);
        // output_head must be accessed only in this block
        {
            std::lock_guard<decltype(this->mutex_output)> lock(this->mutex_output);
            if(this->output_head.index >= this->block_size){
                this->output_head.block->next = new blockStructure2();
                this->output_head = index2{0,this->output_head.block->next};
            }
            this->output_head.block->value[this->output_head.index] = std::move(res);
            this->output_head.index++;
        }
    }}
public:
    IteratorThread(func_t f){
        this->function = f;
        this->input_tail.block = this->input_head.block = new blockStructure1();
        this->input_head.index = this->input_tail.index = 0;
        this->output_tail.block = this->output_head.block = new blockStructure2();
        this->output_head.index = this->output_tail.index = 0;
        this->input_tail.block->next = nullptr;
        this->output_tail.block->next = nullptr;
        this->th = std::thread{&IteratorThread::process,this};
    }
    ~IteratorThread(){
        blockStructure1 *val1 = this->input_tail.block;
        blockStructure2 *val2 = this->output_tail.block;
        {
            std::unique_lock<decltype(this->mutex_input)> lock(this->mutex_input);
            // causes break worker thread
            this->input_tail.block = nullptr;
            // no more input
            this->input_head.block = nullptr;
            // no more output
            this->output_tail.block = nullptr;
            this->condition_.notify_all();
        }
        this->th.join();
        while (val1)
        {
            blockStructure1 *next = val1->next;
            delete val1;
            val1 = next;
        }
        while (val2)
        {
            blockStructure2 *next = val2->next;
            delete val2;
            val2 = next;
        }
    }
    void submit(const T1 &val){
        {
            std::lock_guard<decltype(this->mutex_input)> lock(this->mutex_input);
            if(!this->input_head.block)
                    return;
            if(this->input_head.index >= this->block_size){
                this->input_head.block->next = new blockStructure1();
                this->input_head.index = 0;
                this->input_head.block = this->input_head.block->next;
            }
            this->input_head.block->value[this->input_head.index] = val;
            this->input_head.index++;
        }
        this->condition_.notify_one();
    }
    /// @brief return false if no more callback is available
    /// @param func function pointer required since this class is a helper
    bool processCallback(void (*func)(const T1&)){
        T1 buff;
        {
            std::lock_guard<decltype(this->mutex_output)> lock(this->mutex_output);
            if(!this->output_tail.block)
                return false;
            if(this->output_tail.block == this->output_head.block
            && this->output_tail.index >= this->output_head.index)
                return false;

            // if reached end of block
            if(this->output_tail.index >= this->block_size)
            {
                // technically next block must not be either null nor empty at this point
                blockStructure2 *next = this->output_tail.block->next;
                delete this->output_tail.block;
                this->output_tail.index = 0;
                this->output_tail.block = next;
            }
            buff = std::move(this->output_tail.block->value[this->output_tail.index]);
            this->output_tail.index++;
        }
        // check if entity exists here
        if(true){
            func(buff);
        }
        return true;
    }
};

} // namespace DOTS


#endif // ITERATORTHREAD_HPP
