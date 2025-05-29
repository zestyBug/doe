#if !defined(BITSET_HPP)
#define BITSET_HPP

#include <stdlib.h>
#include <string.h>
#include <bits/algorithmfwd.h>
#include <stdexcept>

#include <typeindex>
#include <typeinfo>

#include "./HashHelper.hpp"

class bitset
{
private:
    // data is stored like this (numbers represent index of bit)
    // 7 6 5 4 3 2 1 0 - 15 14 13 12 11 10 9 8 - 23 22 21 20 19 18 17 16 - ...
    unsigned char * data = nullptr;
    // size in byte
    size_t size = 0;
public:
    bitset(size_t bit_count = 0){
        if(bit_count != 0){
            this->size = (bit_count>>3)+((bit_count&0b111)?1:0);
            this->data = (unsigned char *)calloc(this->size,1);
        }
    }
    bitset(const bitset& src) {
        this->data = (unsigned char *)malloc(src.size);
        this->size = src.size;
        memcpy(this->data,src.data,src.size);
    }
    bitset(bitset&& src) { this->data = src.data;this->size = src.size;src.size = 0; }
    ~bitset(){
        if(this->data)
            free(this->data);
    }
    bitset& operator = (const bitset& src){
        if(this != &src){
            if(this->size >= src.size){
                clear();
            }else{
                if(this->data)
                    free(this->data);
                this->data = (unsigned char *)malloc(src.size);
                this->size = src.size;
            }
            memcpy(this->data,src.data,src.size);
        }
        return *this;
    }
    bitset& operator = (bitset&& src){
        if(this != &src){
            if(this->data)
                free(this->data);
            this->data = src.data;this->size = src.size;
            src.data = nullptr;src.size = 0;
        }
        return *this;
    }
    bool operator [] (size_t index) const {
        if(/*index < 0 || */size == 0) return 0;
        if(index >= capacity()) return 0;
        const unsigned char *buffer = this->data + (index>>3);
        return (*buffer >> (index & 0b111)) & 1;
    }
    void set(const size_t index, const bool val){
        //if(index < 0) return;
        if(index >= capacity())
            this->resize((index>>3) + 1);
        unsigned char *pointer = this->data + (index>>3);

        if(val == false){
            *pointer &= (unsigned char) ~(1 << (index & 0b111));
        }else{
            *pointer |= 1 << (index & 0b111);
        }
    }
    template<typename T>
    void set(const T value){
        size_t min_size = 0;
        for (size_t i = 0; i < sizeof(T); i++)
            if( value & (0xFF<<(8*i)) )
                min_size = i+1;
        resize(min_size);
        clear();
        for (size_t i = 0; i < min_size; i++)
            this->data[i] = (unsigned char)((value >> (8*i)) & 0xFF);
    }
    template<typename T>
    T get() const {
        const unsigned char min_size = std::min(this->size,sizeof(T));
        T value = 0;
        for(size_t i = 0; i < min_size; i++)
            value |= this->data[i] << (8*i);
        return value;
    }
    // set every bit to 0
    void clear(void){
        memset(this->data,0,this->size);
    }
    // number bit can hold
    size_t capacity() const {
        return this->size * 8;
    }
    // size in byte
    // can only extend array, does not shrink
    void resize(const size_t s){
        if(this->size < s){
            this->size = s;
            if(this->data == nullptr)
                this->data = (unsigned char*)calloc(s,1);
            else{
                this->data = (unsigned char*)realloc(this->data,s);
                memset(this->data+this->size,0,s-this->size);
            }
        }
    }
    // position of last 1 bit as size
    // so returns 0 if there is no 1 bit
    size_t true_size() const {
        size_t index = 0;
        for (size_t i = 0; i < this->size; i++)
        {
            unsigned char buffer = this->data[i];
            if(buffer != 0){
                index = i*8;
                do{
                    index++;
                    buffer = buffer >> 1;
                }while(buffer);
            }
        }
        return index;
    }
    bool all_zero() const {
        for (size_t i = 0; i < this->size; i++)
            if(this->data[i])
                return false;
        return true;
    }
protected:
    struct bitset_iterator {
        size_t bit_index = 0;
        unsigned char * data;

        bool operator*(){
            return (this->data[this->bit_index>>3] >> (this->bit_index & 0b111)) & 1;
        }
        bitset_iterator& operator++(){
            this->bit_index++;
            return *this;
        }
        bool operator!=(bitset_iterator end){
            return bit_index < end.bit_index ? true : false;
        }
    };
public:
    bitset_iterator end() const {return bitset_iterator{this->size * 8,this->data};}
    bitset_iterator begin() const {return bitset_iterator{0,this->data};}

    bool operator==(const bitset & set) const {
        const size_t minimum = std::min(set.size,this->size);

        for (size_t i = 0; i < minimum; i++)
            if(this->data[i] != set.data[i])
                return false;

        if(set.size > this->size){
            for (size_t i = minimum; i < set.size; i++)
                if(0 != set.data[i])
                    return false;
        } else if(set.size < this->size){
            for (size_t i = minimum; i < this->size; i++)
                if(0 != this->data[i])
                    return false;
        }
        return true;
    }
    bool operator!=(const bitset & set) const {
        return !(*this == set);
    }
    bitset operator | (const bitset& val) const {
        bitset ret;
        ret.resize(std::max(this->size,val.size));
        if(this->size >= val.size){
            ret = *this;
            for (size_t i = 0; i < std::min(this->size,val.size); i++)
                ret.data[i] |= val.data[i];
        }else{
            ret = val;
            for (size_t i = 0; i < std::min(this->size,val.size); i++)
                ret.data[i] |= this->data[i];
        }
        return ret;
    }
    bitset operator & (const bitset& val) const {
        bitset ret;
        ret.resize(std::min(this->size,val.size));
        for (size_t i = 0; i < ret.size; i++)
                ret.data[i] = val.data[i] & this->data[i];
        return ret;
    }
    // this & ~val
    bitset and_not(const bitset& val) const {
        bitset ret = *this;
        for (size_t i = 0; i < std::min(ret.size,val.size); i++)
            ret.data[i] &= ~val.data[i];
        return ret;
    }
    // this & val == this
    bool and_equal(const bitset& val) const {
        for (size_t i = 0; i < std::min(this->size,val.size); i++)
            if((this->data[i] & val.data[i]) != this->data[i])
                return false;
        // in case this is bigger than val
        for (size_t i = std::min(this->size,val.size); i < this->size; i++)
            if(this->data[i] != 0)
                return false;
        return true;
    }
    void debug() const {
        for(bool bit:*this){
            printf("%s",bit?"1":"0");
        }
        puts("");
    }
};









template<size_t BYTE>
class bitset_static
{
private:
    // data is stored like this (numbers represent index of bit)
    // 7 6 5 4 3 2 1 0 - 15 14 13 12 11 10 9 8 - 23 22 21 20 19 18 17 16 - ...
    unsigned char data[BYTE];
public:
    bitset_static() = default;
    ~bitset_static() = default;
    bitset_static(const bitset_static& src) = default;
    bitset_static(bitset_static&& src) = default;
    bitset_static& operator = (const bitset_static& src) = default;
    bitset_static& operator = (bitset_static&& src) = default;

    bool operator [] (size_t index) const {
        if(index >= capacity())
            throw std::out_of_range("");
        return ( this->data[index>>3] >> (index & 0b111)) & 1;
    }
    void set(const size_t index, const bool val){
        if(index >= capacity())
            throw std::out_of_range("");
        if(val == false)
            this->data[index>>3] &= (unsigned char) ~(1 << (index & 0b111));
        else
            this->data[index>>3] |= 1 << (index & 0b111);
    }
    template<typename T>
    void set(const T value){
        unsigned char min_size = std::min(BYTE,sizeof(T));
        for (size_t i = 0; i < min_size; i++)
            this->data[i] = (unsigned char)((value >> (8*i)) & 0xFF);
    }
    template<typename T>
    T get() const {
        const unsigned char min_size = std::min(BYTE,sizeof(T));
        T value = 0;
        for(size_t i = 0; i < min_size; i++)
            value |= this->data[i] << (8*i);
        return value;
    }
    // set every bit to 0
    void clear(void){
        memset(this->data,0,BYTE);
    }
    // number bit can hold
    size_t capacity() const {
        return BYTE * 8;
    }
    bool all_zero() const {
        for (size_t i = 0; i < BYTE; i++)
            if(this->data[i])
                return false;
        return true;
    }
protected:
    struct bitset_iterator {
        size_t bit_index = 0;
        const unsigned char * data;

        bool operator*(){
            return (this->data[this->bit_index>>3] >> (this->bit_index & 0b111)) & 1;
        }
        inline bitset_iterator& operator++(){
            this->bit_index++;
            return *this;
        }
        inline bool operator!=(bitset_iterator end){
            return bit_index < end.bit_index ? true : false;
        }
    };
public:
    // note: Shouldnt iterator iterate over 1 bit index?
    bitset_iterator begin() const {return bitset_iterator{0,this->data};}
    bitset_iterator end()   const {return bitset_iterator{this->capacity(),this->data};}

    inline bool operator==(const bitset_static & set) const {
        if(!memcmp(this->data,set.data,BYTE))
            return true;
        return false;
    }
    inline bool operator!=(const bitset_static & set) const {
        return !(*this == set);
    }
    bitset_static operator | (const bitset_static& val) const {
        bitset_static ret;
        for (size_t i = 0; i < BYTE; i++)
                ret.data[i]= this->data[i] | val.data[i];
        return ret;
    }
    bitset_static operator & (const bitset_static& val) const {
        bitset_static ret;
        for (size_t i = 0; i < BYTE; i++)
            ret.data[i]= this->data[i] & val.data[i];
        return ret;
    }
    // this & ~val
    bitset_static and_not(const bitset_static& val) const {
        bitset_static ret;
        for (size_t i = 0; i < BYTE; i++)
                ret.data[i]= this->data[i] & ~(val.data[i]);
        return ret;
    }
    // this & val == this
    bool and_equal(const bitset_static& val) const {
        for (size_t i = 0; i < BYTE; i++)
            if((this->data[i] & val.data[i]) != this->data[i])
                return false;
        return true;
    }
    void debug() const {
        for(bool bit:*this){
            printf("%s",bit?"1":"0");
        }
        puts("");
    }
    // Helper: Hash a vector of type_index (order-insensitive)
    inline uint64_t hash() const {
        return HashHelper::FNV1A64(this->data,BYTE);
    }
protected:
};


#endif // BITSET_HPP
