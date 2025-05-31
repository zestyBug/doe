#if !defined(STATICARRAY_HPP)
#define STATICARRAY_HPP

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdexcept>

template<typename T, uint32_t S>
class static_array
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
    uint32_t count = 0;
public:
    static_assert(S > 0);
    static_array(){}
    static_array(const static_array& value){
        for (size_t i = 0; i < value.count; i++)
            new (_data.array + i) Type(value._data.array[i]);
        count=value.count;
    }
    static_array& operator = (const static_array& value){
        if(this!=&value){
            for (size_t i = 0; i < count; i++)
                _data.array[i].~Type();
            for (size_t i = 0; i < value.count; i++)
                new (_data.array + i) Type(value._data.array[i]);
            count=value.count;
        }
        return *this;
    }
    void clear(){
        for (size_t i = 0; i < count; i++)
            _data.array[i].~Type();
        count = 0;
    }
    ~static_array(){
        clear();
    }

    inline uint32_t capacity()const {return S;}
    inline uint32_t size()const {return this->count;}
    inline uint32_t size_byte()const {return this->count*sizeof(Type);}
    inline bool     empty()const {return this->count < 1;}
    inline bool     full()const {return this->count == S;}
    inline uint32_t remainding()const {return S - this->count;}
        
    inline Type*  begin(){return this->_data.array;}
    inline Type*  end(){return this->_data.array + S;}
    inline const Type* begin()const {return this->_data.array;}
    inline const Type* end()const {return this->_data.array + S;}
    inline const T* data () const {return this->_data.array;}
    inline T* data () {return this->_data.array;}

    template<typename ... Args>
    inline void emplace_back(Args ... arg){
        if(full())
            throw std::out_of_range("at(): array full");
        new (&(this->_data.array[this->count++])) Type(arg...);
    }
    inline void push_back(const Type& v){
        if(full())
            throw std::out_of_range("at(): array full");
        new (&(this->_data.array[this->count++])) Type(v);
    }
    inline void pop_back() {
        if(this->count > 0){
            this->count--;
            this->_data.array[this->count].~Type();
        }
    }
    inline Type& at(uint32_t index) {
        if(S <= index)
            throw std::out_of_range("at(): invalid index");
        return this->_data.array[index];
    }
    inline const Type& at(uint32_t index) const {
        if(S <= index)
            throw std::out_of_range("at(): invalid index");
        return this->_data.array[index];
    }
    inline Type& operator[](uint32_t index) {
        return _data.array[index];
    }
    inline const Type& operator[](uint32_t index) const {
        return _data.array[index];
    }
    inline void pop_swap_at_back(uint32_t index) {
        if(count <= index)
            throw std::out_of_range("pop(): invalid index");
        _data.array[index].~Type();
        count--;
        if(count != index)
            _data.array[count] = _data.array[index];
    }
    // pop without filling the created empty space in middle of array
    inline void pop2(uint32_t index) {
        if(count <= index)
            throw std::out_of_range("pop(): invalid index");
        _data.array[index].~Type();
    }
};

#endif // STATICARRAY_HPP
