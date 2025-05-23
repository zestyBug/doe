#if !defined(STATICARRAY_HPP)
#define STATICARRAY_HPP

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

template<typename T, size_t S>
class StaticArray
{
    using Type = T;
private:
    union container
    {
        Type array[S];
        bool _;
        container(){
            _ = false;
        }
    };
    container _data{};
    size_t count = 0;
public:
    static_assert(S > 0);
    StaticArray(){}
    ~StaticArray(){}
    inline size_t capacity()const {return S;}
    inline size_t size()const {return this->count;}
    inline bool   empty()const {return this->count < 1;}
    inline bool   full()const {return this->count == S;}
    inline Type*  begin(){return this->_data.array;}
    inline Type*  end(){return this->_data.array + S;}
    inline const Type* begin()const {return this->_data.array;}
    inline const Type* end()const {return this->_data.array + S;}
    inline T* data ()const {return this->_data.array;}
    template<typename ... Args>
    inline void emplace(Args ... arg){
        assert(!this->full());
        new (&(this->_data.array[this->count++])) Type(arg...);
    }
    inline void push(const Type& v){
        assert(!this->full());
        new (&(this->_data.array[this->count++])) Type(v);
    }
    inline void pop_back() {
        if(this->count > 0){
            this->count--;
            this->_data.array[this->count].~Type();
        }
    }
    inline Type& at(size_t index) {
        assert(S > index);
        return this->_data.array[index];
    }
    inline Type& operator[](size_t index) {
        return this->_data.array[index];
    }
    inline const Type& operator[](size_t index) const {
        return this->_data.array[index];
    }
    inline void pop(size_t index) {
        if(this->count > index)
        {
            this->_data.array[index].~Type();
            this->count--;
            if(this->count > 0 && this->count != index)
                memcpy(this->_data.array + index, this->_data.array + this->count, sizeof(Type));
        }
    }
    // pop without filling the created empty space in middle of array
    inline void pop2(size_t index) {
        if(this->count > index)
            this->_data.array[index].~Type();
    }
};

#endif // STATICARRAY_HPP
