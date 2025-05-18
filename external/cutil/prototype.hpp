#if !defined(PROTOTYPE_HPP)
#define PROTOTYPE_HPP
#include <stdio.h>

class prototype
{
private:
    int sample=-1;
public:
    prototype(int id=-1):sample{id}{
        printf("prototype(%p:%d)\n",this,this->sample);
    }
    prototype(const prototype& v):sample{v.sample}{
        printf("operator{%p}::operator(prototype&& {%p:%d})\n",this,&v,v.sample);
    }
    prototype(prototype&& v):sample{v.sample}{
        printf("operator{%p}::operator(prototype&& {%p:%d})\n",this,&v,v.sample);
    }

    prototype& operator = (const prototype& v){
        if(this != &v){
            printf("operator{%p:%d}::operator = (const prototype& {%p:%d})\n",this,this->sample,&v,v.sample);
            this->sample = v.sample;
        }
        return *this;
    }
    prototype& operator = (prototype&& v){
        if(this != &v){
            printf("operator{%p:%d}::operator = (prototype&& {%p:%d})\n",this,this->sample,&v,v.sample);
            this->sample = v.sample;
            v.sample = -1;
        }
        return *this;
    }
    ~prototype(){
        printf("operator{%p:%d}::~prototype()\n",this,this->sample);
    }
};


#endif // PROTOTYPE_HPP
