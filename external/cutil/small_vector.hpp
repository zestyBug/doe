#if !defined(SMALLVECTOR_HPP)
#define SMALLVECTOR_HPP

#include <memory>
#include <string.h>
#include <stdlib.h>
#include "basics.hpp"
template<typename T,unsigned int S>
class small_vector{
    unsigned int _count=0,_capacity=0;
    union block
    {
        T small[S];
        T *large;
        block():large{nullptr}{}
    };
    block data{};
public:
    small_vector():_count{0},_capacity{S} {
        //
    }
    small_vector(const small_vector &obj){
        _count = 0;
        _capacity = S;
        *this = obj;
    }
    small_vector& operator = (const small_vector &obj){
        if(this != &obj){
            T *ptr1,*ptr2;
            if(obj._capacity <= S){
                this->~small_vector();
                ptr1 = data.small;
                ptr2 = obj.data.small;
                _count = obj._count;
                _capacity = S;
            }else{
                if(_capacity < obj._count){
                    this->~small_vector();
                    _count = obj._count;
                    _capacity = obj._count;
                    data.large = allocator<T>().allocate(obj._count);
                }else{
                    allocator<T>().destroy(data.large,_count);
                    _count = obj._count;
                }
                ptr1 = data.large;
                ptr2 = obj.data.large;
            }
            for (size_t i = 0; i < obj._count; i++)
                new (ptr1 + i) T(ptr2 + 1);
        }
        return *this;
    };
    small_vector(small_vector &&obj){
        _count = 0;
        _capacity = S;
        *this = std::move(obj);
    };
    small_vector& operator = (small_vector &&obj){
        if(this != &obj){
            this->~small_vector();
            T *ptr1,*ptr2;
            _count = obj._count;
            if(obj._capacity <= S){
                _capacity = S;
                memcpy(data.small,obj.data.small,sizeof(T)*obj._count);
            }else{
                _capacity = obj._capacity;
                data.large = obj.data.large;
            }
            obj._count = 0;
            obj._capacity = S;
        }
        return *this;
    };
    small_vector(unsigned int count,const T& val = T()):_count{count} {
        T *ptr;
        if(count <= S){
            ptr = this->data.small;
            _capacity = S;
        }else{
            this->_capacity = count;
            ptr = this->data.large = allocator<T>().allocate(count);
        }
        for (size_t i = 0; i < count; i++)
            new (ptr + i) T(val);
        
    }
    ~small_vector(){
        if(this->_capacity <= S){
            allocator<T>().destroy(data.small,_count);
        }else{
            allocator<T>().destroy(data.large,_count);
            allocator<T>().deallocate(data.large);
        }
    }

    void expand(){
        T *ptr1 = _capacity<=S ? data.small : data.large;
        _capacity = std::max( _capacity*2 , S+2);
        T *ptr2 = allocator<T>().allocate(_capacity);
        memcpy(ptr2,ptr1,_count*sizeof(T));
        data.large = ptr2;
    }


    void push_back(const T& val){
        if(_count >= _capacity){
            expand();
        }
        
        if(this->_capacity <= S){
            new (data.small + _count) T(val);
        }else{
            new (data.large + _count) T(val);
        }
        ++_count;
    }
    /* void remove(unsigned int index){
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
    } */
    T& operator [](unsigned int index){
        if(index >= this->_count)
            throw std::out_of_range("operator []():");
        if(this->_capacity <= S){
            return this->data.small[index];
        }else{
            return this->data.large[index];
        }
    }
    inline bool empty() const { return _count == 0; }
    inline unsigned int size() const { return _count; }
    inline unsigned int capacity() const { return _capacity; }

    inline T* begin()             {return this->_capacity <= S ? this->data.small : this->data.large;}
    inline const T* begin() const {return this->_capacity <= S ? this->data.small : this->data.large;}
    inline T* end()               {return this->_capacity <= S ? this->data.small+this->size : this->data.large+this->size;}
    inline const T* end() const   {return this->_capacity <= S ? this->data.small+this->size : this->data.large+this->size;}
};

#endif // SMALLVECTOR_HPP
