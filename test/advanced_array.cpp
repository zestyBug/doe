#include "advanced_array/advanced_array.hpp"
#include <iostream>
#include <stdlib.h>
#include <time.h>

static int counter=0;

class test
{
private:
    int *val,*not_used=nullptr;
public:
    test():val{new int(0)}{
        counter++;
        //std::cout << "test()" << std::endl;
    }
    test(const int v):val{new int(v)}{
        counter++;
        //std::cout << "test(const int)" << std::endl;
    }
    test(const test& obj):val{new int(*obj.val)}{
        counter++;
        //std::cout << "test(const test&)" << std::endl;
    }
    test& operator = (const test& obj) {
        if(&obj != this){
            if(this->val) {
                delete this->val;
                counter--;
                this->val=nullptr;
            }
            if(obj.val) {
                this->val = new int(*obj.val);
                counter++;
            }
            //std::cout << "test& operator = (const test&)" << std::endl;
        }
        return *this;
    }
    test(test&& obj){
        this->val = obj.val;
        obj.val=nullptr;
        //std::cout << "test(test&&)" << std::endl;
    }
    test& operator = (test&& obj) {
        if(&obj != this){
            if(this->val) {
                delete this->val;
                counter--;
                this->val=nullptr;
            }
            this->val = obj.val;
            obj.val=0;
            //std::cout << "test& operator = (test&&)" << std::endl;
        }
        return *this;
    }
    ~test(){
        if(this->val) {
            delete this->val;
            counter--;
            this->val=nullptr;
        }
        //std::cout << "~test()" << std::endl;
    }
    int operator ()() const {
        return *val;
    }
};



class Range final
{
    using value = ssize_t;
    const value FROM;
    const value TO;
    const value STEP;
    class Iterator final
    {
        Range::value _value;
        const Range::value _step;
    public:
        Iterator(const Range::value value,const Range::value step):_value{value},_step{step} {}
        Iterator(const Iterator& obj) = default;
        Iterator& operator = (const Iterator& obj) = default;
        // a bit tricky!!!
        constexpr bool operator != (const Iterator &obj) const {
            // returning false, breaks the loop
            return _step > 0 ? this->_value < obj._value : this->_value > obj._value;
        }
        Iterator & operator++() {return _value+=_step, *this;}
        Iterator & operator--() {return _value-=_step, *this;}
        Iterator   operator+(const Iterator b) const {return {this->_value+b._value,this->_step};}
        Iterator   operator-() const {return {-this->_value,this->_step};}
        Iterator & operator+=(const Iterator& b) {this->_value+=b._value;return *this;}
        Iterator & operator-=(const Iterator& b) {return (*this += (-b));}
        constexpr Range::value operator*() const {return this->_value;}
        constexpr Range::value operator[](const ssize_t i) const {return this->_value + this->_step * i;}
    };
public:
    Range(const ssize_t from, const ssize_t to, const ssize_t step=1):FROM{from},TO{to},STEP{step} {};
    Iterator begin() const {return {FROM,STEP};}
    const Iterator end() const {return {TO,STEP};}
    Iterator rbegin() const {return {TO,-STEP};}
    const Iterator rend() const {return {FROM,-STEP};}
};

int main(int argc, char const *argv[])
{
    srandom((int)time(NULL));
    /* code */
    {
        advanced_array::registry<test> reg;

        for(auto i:Range(0,30)){
            for(auto i:Range(0,2)){
                advanced_array::entity ent=reg.create();
                if( advanced_array::to_integral(ent) == 0)
                {
                    std::cout << "*" << std::endl;
                }
                reg.emplace(ent,i);
            }
            advanced_array::basic_view<advanced_array::entity,test> view = reg.view();
            for(auto &ent : view)
            {
                //test& val = view.get(ent);
                if((random() & 0b1111) == 0b1111){
                    reg.destroy(ent);
                    continue;
                }
                //std::cout << entt::to_integral(ent) << std::endl;
            }
        }
        auto view = reg.view();
        for(auto &ent : view)
        {
            test& val = view.get(ent);
            printf("%7X %p\n",advanced_array::to_integral(ent),&val);
            reg.destroy(ent);
        }
    }

    return 0;
}