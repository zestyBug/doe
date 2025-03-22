#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "cutil/bitset.hpp"

namespace DOTS
{

    class Archetype final {
        struct single_component
        {
            // if pointer is null, then the given component index
            // does not exist in the given archetype
            void* pointer = nullptr;
            comp_info info;
        };

        std::vector<single_component> components;
        Entity* entity_id=nullptr;
        size_t size=0;
        size_t capacity=0;
        friend class Register;
        // max entity in an archetype, a register may have different max entity count
        static constexpr size_t max_entity_count = sizeof(size_t)*8 - 1;
    public:
        Archetype(){};
        Archetype(Archetype&& obj){
            if(obj.capacity){
                this->components = std::move(obj.components);
                this->entity_id = obj.entity_id;
                this->size = obj.size;
                this->capacity = obj.capacity;
                obj.entity_id = nullptr;
                obj.size = 0;
                obj.capacity = 0;
            }
        };
        Archetype(const Archetype& obj) = delete;

        /// @param isize: initial entity list size
        void initialize(const bitset& types,const size_t isize=4) {
            this->entity_id = (Entity*)malloc(sizeof(Entity) * isize);
            this->capacity  = isize;
            // also zero component entities are alowed
            const size_t component_count = types.true_size();
            components.reserve(component_count);

            for(size_t i=0;i < component_count;i++)
                if(types[i])
                    this->initialize_component(i);
        }
    protected:
        void initialize_component(const typeid_t id) {
            const comp_info info = type_id(id);
            const size_t s = info.size * this->capacity;
            //printf("Debug: malloc(%llu)\n",s);
            this->components[info.index].pointer = malloc(s);
            this->components[info.index].info = info;
        }
    public:
        // recives id of entity (index of entity in registers enity array + version)
        // returns a new valid index in this archetype
        entityId_t allocate_entity(const Entity index) {
            assert(this->size < max_entity_count);

            if(this->size < this->capacity){
                this->entity_id[this->size] = index;
                return (entityId_t)(this->size++);
            }else{
                this->capacity *= 2;
                for (auto& com:components)
                    if(com.pointer)
                        //printf("Debug: realloc(%llu >> %llu)\n",old_size,old_size*2);
                        assert(com.pointer = realloc(com.pointer, com.info.size * this->capacity));

                assert(entity_id = (Entity*)realloc(entity_id, sizeof(Entity) * this->capacity));
                // TODO: covers wierd scenarios
                return allocate_entity(index);
            }
        }

        // recives index in components array
        // returns id of entity that filled the empty space in array or invalid index
        Entity destroy_entity(const entityId_t index) {
            assert(this->size > index);

            this->size--;


            for (auto& com:components)
                    if(com.pointer){
                        char * const ptr = (char*)(com.pointer) + (index * com.info.size);
                        if(com.info.destructor)
                            com.info.destructor(ptr);
                        if(index != this->size){
                            char * const last = (char*)(com.pointer) + (this->size * com.info.size);
                            memcpy(ptr,last,com.info.size);
                        }
                    }

            if(index != this->size){
                return (this->entity_id[index] = this->entity_id[this->size]);
            }else{
                return Entity::null;
            }
        }
    protected:
        // same as destroy() without calling destructor functions
        Entity destroy2(const entityId_t index) {
            assert(this->size > index);
            this->size--;
            for (auto& com:components)
                if(com.pointer){
                    char * const ptr = (char*)(com.pointer) + (index * com.info.size);
                    if(index != this->size){
                        char * const last = (char*)(com.pointer) + (this->size * com.info.size);
                        memcpy(ptr,last,com.info.size);
                    }
                }
            if(index != this->size){
                return (this->entity_id[index] = this->entity_id[this->size]);
            }else{
                return Entity::null;
            }
        }
        // destroy all entities in this given archetype and itself
        void destroy(){
            for (auto& com:components)
                if(com.pointer){
                    if(com.info.destructor)
                        for(size_t i = 0; i < this->size; i++)
                            com.info.destructor((char*)(com.pointer) + (i * com.info.size));
                    free(com.pointer);
                }
            components.clear();
            if(this->entity_id)
                free(this->entity_id);
            this->entity_id = nullptr;
            this->size=0;
            this->capacity=0;
        }
    public:
        ~Archetype(){
            this->destroy();
        }
    };

} // namespace DOTS


#endif // ARCHTYPE_HPP
