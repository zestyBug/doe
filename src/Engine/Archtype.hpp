#if !defined(ARCHTYPE_HPP)
#define ARCHTYPE_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <string.h>
#include <assert.h>
#include <stdio.h>

namespace DOTS
{
    
    struct Archtype final {
        std::array<void*,33> components;
        std::array<comp_info,33> components_info;
        size_t capacity;
        size_t size;
        
        Archtype():capacity(0),size(0){
            memset(&this->components,0,sizeof(this->components));
        };

        void initialize(compid_t types_bitmask,const size_t initSize=4) {
            this->capacity = initSize;
            assert(this->capacity > 0);

            for(int bitmask=1,i=0;i < (int)(sizeof(types_bitmask)*8);i++){
                if(types_bitmask & bitmask)
                    initialize_component(i);
                bitmask <<= 1;
            }
            //printf("Debug: malloc(%llu)\n",this->capacity*sizeof(Entity));
            this->components[32] = malloc(this->capacity*sizeof(Entity));
            this->components_info[32] = comp_info{32,sizeof(Entity),nullptr};
        }

        void initialize_component(const compid_t type) {
            const comp_info info = type_id(type);
            const size_t s = info.size * this->capacity;
            //printf("Debug: malloc(%llu)\n",s);
            this->components[info.index] = malloc(s);
            this->components_info[info.index] = info;
        }

        // recives id of entity (index of entity in registers enity array + version)
        // returns a new valid index in this archtype
        entity_t allocate(const Entity index) {
            assert(this->size < 0xffffff);
            if(this->size < this->capacity){
                ((entity_t*)this->components[32])[this->size] = index;
                return this->size++;
            }else{
                for (size_t i = 0; i < 33; i++)
                    if(components[i]) {
                        const size_t old_size = this->components_info[i].size * this->capacity;
                        //printf("Debug: realloc(%llu >> %llu)\n",old_size,old_size*2);
                        assert(this->components[i] = realloc(this->components[i],old_size*2));
                    }
                this->capacity *= 2;
                // TODO: covers wierd scenarios
                return allocate(index);
            }
        }

        // recives index in components array
        // returns id of entity that filled the empty space in array or invalid index
        Entity destroy(const Entity index) {
            assert(this->size > index);
            this->size--;
            for (size_t i = 0; i < 33; i++)
                if(components[i]){
                    const comp_info info = this->components_info[i];
                    char * const ptr = (char*)(components[i]) + (index * info.size);
                    if(info.destructor)
                       info.destructor(ptr);
                    if(index != this->size){
                        char * const last = (char*)(this->components[i]) + (this->size * info.size);
                        memcpy(ptr,last,info.size);
                    }
                }
            if(index != this->size){
                return ((entity_t*)this->components[32])[index];
            }else{
                return 0xffffff;
            }
        }
        // same as destroy() without calling destructor functions
        Entity destroy2(const Entity index) {
            assert(this->size > index);
            this->size--;
            const size_t size_buffer = this->size;
            if(index != size_buffer)
                for (size_t i = 0; i < 33; i++)
                    if(char * const ptr = (char*)(this->components[i]);ptr){
                            const size_t comonent_size = this->components_info[i].size;
                            memcpy(ptr+(index*comonent_size) , ptr+(size_buffer*comonent_size) , comonent_size);
                    }
            if(index != size_buffer){
                return ((entity_t*)this->components[32])[index];
            }else{
                return 0xffffff;
            }
        }
        // destroy all entities in this given archtype and itself
        void destroy(){
            for (size_t comp = 0; comp < 33; comp++)
                if(this->components[comp] != nullptr) {
                    //printf("Debug: free(%llu)\n",this->components_info[comp].size * this->capacity);
                    if(this->components_info[comp].destructor)
                        for (size_t i = 0; i < this->size; i++)
                        {
                            void *ptr = (char*)(this->components[comp]) + (this->components_info[comp].size * i);
                            this->components_info[comp].destructor(ptr);
                        }
                    free(this->components[comp]);
                }
            memset(this->components.data(),0,sizeof(this->components));
            this->size=0;
            this->capacity=0;
        }
        ~Archtype(){
            this->destroy();
        }
    };

} // namespace DOTS


#endif // ARCHTYPE_HPP
