#if !defined(PROTOTYPE_HPP)
#define PROTOTYPE_HPP
#include <stdio.h>

class prototype
{
private:
    int sample=-1;
    static int counter;
public:
    static int getCount(){
        return counter;
    }

    prototype(int id=-1):sample{id}{
    #ifdef VERBOSE
        printf("prototype(%p:%d)\n",this,this->sample);
    #endif
        counter++;
    }
    prototype(const prototype& v):sample{v.sample}{
    #ifdef VERBOSE
        printf("operator{%p}::operator(prototype&& {%p:%d})\n",this,&v,v.sample);
    #endif
        counter++;
    }
    prototype(prototype&& v):sample{v.sample}{
    #ifdef VERBOSE
        printf("operator{%p}::operator(prototype&& {%p:%d})\n",this,&v,v.sample);
    #endif
        counter++;
    }

    prototype& operator = (const prototype& v){
        if(this != &v){
        #ifdef VERBOSE
            printf("operator{%p:%d}::operator = (const prototype& {%p:%d})\n",this,this->sample,&v,v.sample);
        #endif
            this->sample = v.sample;
            counter++;
        }
        return *this;
    }
    prototype& operator = (prototype&& v){
        if(this != &v){
        #ifdef VERBOSE
            printf("operator{%p:%d}::operator = (prototype&& {%p:%d})\n",this,this->sample,&v,v.sample);
        #endif
            this->sample = v.sample;
            v.sample = -1;
            counter++;
        }
        return *this;
    }
    ~prototype(){
    #ifdef VERBOSE
        printf("operator{%p:%d}::~prototype()\n",this,this->sample);
    #endif
        counter--;
    }
};


#endif // PROTOTYPE_HPP
