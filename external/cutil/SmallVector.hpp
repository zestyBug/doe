#if !defined(SMALLVECTOR_HPP)
#define SMALLVECTOR_HPP

#include <memory>
#include <string.h>
#include <stdlib.h>

template<typename T,unsigned int S>
class SmallVector{
    unsigned int _size=0,_capacity=0;
    union block
    {
        T small[S];
        T *large;
        block():large{nullptr}{}
    } data;
public:
    SmallVector():_capacity{S}, _size{0} {
        //
    }
    SmallVector(const SmallVector &obj):_size{obj._size},_capacity{obj._capacity}
    {
        if(obj._capacity <= S){
            memcpy(&this->data,&obj.data,obj._size*sizeof(T));
        }else{
            this->data.large = (T*)malloc(obj._capacity*sizeof(T));
            memcpy(&this->data.large,&obj.data.large,obj._size*sizeof(T));
        }
    }
    SmallVector(SmallVector &&obj){
        this->_size = obj._size;
        this->_capacity =  obj._capacity;
        memcpy(&this->data,&obj.data,sizeof(this->data));
        obj._size = 0;
        obj._capacity = S;
    };
    SmallVector(unsigned int count,const T& val = T()):_size{count} {
        if(count > 0){
            if(count <= S){
                for (size_t i = 0; i < count; i++)
                    new (this->data.small + i) T(val);
            }else{
                this->_capacity = 1;
                for (; (this->_capacity<0b10000000000000000000) && (this->_capacity < count);)
                    this->_capacity<<=1;
                assert(count <= this->_capacity);
                this->data.large = malloc(this->_capacity * sizeof(T));
                for (size_t i = 0; i < count; i++)
                    new (this->data.large + i) T(val);
            }
        }
    }
    ~SmallVector(){
        if(this->_capacity <= S){
            for (size_t i = 0; i < _size; i++)
                this->data.small[i].~T();
        }else{
            for (size_t i = 0; i < _size; i++)
                this->data.large[i].~T();
            free(this->data.large);
        }
    }
    void push_back(const T& val){
        if(this->_size == S){
            this->_capacity = 1;
            while(!(S < this->_capacity) && (this->_capacity < 0b10000000000000000))
                this->_capacity<<=1;
            T *ptr = malloc(this->_capacity);
            memcpy(ptr,this->data.small,S);
            this->data.large = ptr;
        }else if(this->_size < S){
            new (this->data.small + this->_size) T(val);
        }else{
            if(this->_size < this->_capacity){
                new (this->data.large + this->_size) T(val);
            }else{
                this->_capacity <<= 1;
                assert(this->_capacity < 0b10000000000000000 && this->_size < this->_capacity);
                T *ptr = malloc(this->_capacity);
                memcpy(ptr,this->data.large,this->_size * sizeof(T));
                free(this->data.large);
                this->data.large = ptr;
            }
        }
        this->_size++;
    }
    void remove(unsigned int index){
        assert(index < this->_size);
        T* ptr;
        if(this->_capacity <= S)
            ptr = this->data.small;
        else
            ptr = this->data.large;
        ptr[index].~T();
        this->_size--;
        if(index < this->_size){
            memcpy(ptr+index, ptr+this->_size, sizeof(T));
        }
    }
    T& operator [](unsigned int index){
        assert(index < this->_size);
        if(this->_capacity <= S){
            return this->data.small[index];
        }else{
            return this->data.large[index];
        }
    }
    inline bool empty() const { return this->_size == 0; }
    inline unsigned int size() const { return this->_size; }
    inline unsigned int capacity() const { return this->_capacity; }

    inline T* begin()             {return this->_capacity <= S ? this->data.small : this->data.large;}
    inline const T* begin() const {return this->_capacity <= S ? this->data.small : this->data.large;}
    inline T* end()               {return this->_capacity <= S ? this->data.small+this->size : this->data.large+this->size;}
    inline const T* end() const   {return this->_capacity <= S ? this->data.small+this->size : this->data.large+this->size;}
};

#endif // SMALLVECTOR_HPP
