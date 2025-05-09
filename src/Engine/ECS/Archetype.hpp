#include <stdint.h>
#include <vector>
#include <array>
#include <utility>
#include "defs.hpp"
#if !defined(ARCHETYPE_HPP)
#define ARCHETYPE_HPP

namespace DOTS
{
    class Archetype final
    {
    protected:
        struct chunk {
            void *memory = nullptr;
            size_t count = 0;
            chunk(const size_t s){
                this->memory = malloc(s);
                //printf("New memorypool at: %p\n",this->memory);
            }
            chunk(const chunk& ) = delete;
            chunk& operator = (const chunk& ) = delete;
            chunk& operator = (chunk&& obj){
                if(this != &obj){
                    this->memory = obj.memory;
                    this->count = obj.count;
                    obj.memory = nullptr;
                    obj.count = 0;
                }
                return *this;
            }
            chunk(chunk&& obj){
                this->memory = obj.memory;
                this->count = obj.count;
                obj.memory=nullptr;
                obj.count=0;
            }
            ~chunk(){
                if(this->memory){
                    //printf("free(%p)\n",this->memory);
                    free(this->memory);
                    this->memory = nullptr;
                }
            }
        };
        friend class Register;
        static constexpr size_t CHUNK_SIZE = 4096;
        // invalid index and max entity number
        static constexpr entityIndex_t INVALID_INDEX = 0xffffff;
        // maximum number of entities that can be fit into a single chunk
        size_t chunk_capacity = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on last chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<chunk> chunks{};
        std::vector<Entity> entities_id{};
        // list of information about every component that exist in chunks
        // index is sorted as same sort as components in chunks
        // optimal for 16 component per archtype or less
        // otherwise use a std::map
        std::vector<std::pair<typeid_t,size_t>> components_index{};
    public:
        Archetype(/* args */) = default;
        Archetype(const Archetype&) = delete;
        Archetype(Archetype&&) = default;

        // estimated capacity
        // check before accessing an unallocated archtype
        size_t capacity() const {
            return chunk_capacity * chunks.capacity();
        }

        bool empty() const {
            return this->chunks.empty();
        }

        /// @param isize: initial entity list size
        void initialize(const archtype_bitset& types) {
            {
                size_t component_count = 0;
                //size_t last_bit=0;
                size_t size_sum = 0;
                for(size_t i=0;i < types.capacity();i++)
                    if(types[i]){
                        //last_bit = i;
                        component_count++;
                        size_sum += rtti[i].size;
                    }

                if(!size_sum)
                    throw std::length_error("zero sized archetypes are not supported yet");
                //last_bit++;
                this->chunk_capacity = this->CHUNK_SIZE / size_sum;
                if(this->chunk_capacity < 1)
                    throw std::length_error("LARGE COMPONENTS! X(");
                this->chunks.reserve(8);
                this->components_index.reserve(component_count);
                this->entities_id.reserve(8);
            }
            {
                size_t size_sum = 0;
                for(size_t i=0;i < types.capacity();i++)
                    if(types[i]){
                        this->components_index.emplace_back(i,size_sum*this->chunk_capacity);
                        size_sum += rtti[i].size;
                    }
            }
        }



        void destroy(){
            for(chunk &ck:chunks)
            {
                for(const auto& c_info:this->components_index){
                    const auto info = rtti[c_info.first];
                    uint8_t *mem = (uint8_t*)ck.memory;
                    const uint8_t *dest = (uint8_t *)mem + ck.count * info.size;
                    for(;mem<dest;mem+=info.size)
                        info.destructor(mem);
                }

                ck.count = 0;
            }
            this->chunk_capacity = 0;
            this->chunks.clear();
            this->components_index.clear();
            this->entities_id.clear();
        }

        entityIndex_t createEntity(Entity e){
            entityIndex_t res = 0;
            chunk* last_chk = nullptr;

            if(this->chunks.size() == 0 || this->chunks.back().count == this->chunk_capacity)
                this->chunks.emplace_back(this->CHUNK_SIZE);
            last_chk = &this->chunks.back();
            res = this->entities_id.size();
            if(res >= INVALID_INDEX)
                throw std::out_of_range("entity count limit reached");
            this->entities_id.emplace_back(e);
            last_chk->count++;
            return res;
        }
        // remove an entity from archtype and returns entity value of 
        // the entity that has replaced it
        Entity destroyEntity(entityIndex_t entity_index, bool destruction = true){
            if(entity_index >= this->entities_id.size())
                throw std::runtime_error("deleting entity that doesnt exist");
            if(0 >= this->chunks.size())
                throw std::runtime_error("delete operation on an empty archtype");


            entityIndex_t in_chunk_index;
            uint8_t* old_chk_memory;
            {
                chunk& old_chk = this->chunks.at(this->getChunkIndex(entity_index));
                in_chunk_index = this->getEntityIndex(entity_index);
                assert(in_chunk_index < old_chk.count && "destroying already destroyed entity! see version for more info.");
                old_chk_memory = (uint8_t*) old_chk.memory;
            }

            // last entity in archetype


            entityIndex_t last_in_chunk_index;
            const uint8_t* last_chk_memory;
            {
                chunk& last_chk = this->chunks.back();
                if(last_chk.count == 0)
                    throw std::runtime_error("empty chunk in archtype!");
                // act as index of it
                last_chk.count--;
                last_in_chunk_index = last_chk.count;
                last_chk_memory = (uint8_t*) last_chk.memory;
            }


            //const entityIndex_t last_index = (this->chunks.size()-1) * this->chunk_capacity + last_chk.count;

            if(destruction)
                for(size_t i=0;i<this->components_index.size();i++)
                {
                    const auto& info = this->components_index[i];
                    uint8_t * const ptr1 = old_chk_memory + info.second + (rtti[info.first].size * in_chunk_index);
                    if(rtti[info.first].destructor)
                        rtti[info.first].destructor(ptr1);
                }


            Entity last_entity = this->entities_id.back();
            this->entities_id.pop_back();

            // if last entity is the deleted entity
            if(last_chk_memory == old_chk_memory && last_in_chunk_index == in_chunk_index){
                last_entity = Entity();
                goto END;
            }

            for(size_t i=0;i<this->components_index.size();i++){
                const auto& info = this->components_index.at(i);
                      uint8_t *ptr1 = old_chk_memory  + info.second + (rtti[info.first].size*     in_chunk_index);
                const uint8_t *ptr2 = last_chk_memory + info.second + (rtti[info.first].size*last_in_chunk_index);
                memcpy(ptr1,ptr2,rtti[info.first].size);
            }
            this->entities_id.at(entity_index) = last_entity;

            END:

            // 0 means last element on the chunk is either moved to
            // the deleted entity position or is the deleted entity itself
            // so chunk must be cleared
            if(last_in_chunk_index == 0)
                this->chunks.pop_back();
            return last_entity;
        }
        // requesting component that does not exist results undefined behaviour
        void* getComponent(typeid_t component_id,entityIndex_t entity_index){
            assert(entity_index < this->entities_id.size());
            chunk& chk = this->chunks.at(getChunkIndex(entity_index));
            const entityIndex_t index = getEntityIndex(entity_index);
            assert(index < chk.count && "destroying already destroyed entity! see version for more info.");
            
            return (uint8_t*)chk.memory + this->getOffset(component_id) + rtti[component_id].size*index;
        }
        // requesting component that does not exist results undefined behaviour
        template<typename T>
        T* getComponent(entityIndex_t entity_index){
            assert(entity_index < this->entities_id.size());
            chunk& chk = this->chunks.at(getChunkIndex(entity_index));
            const entityIndex_t index = getEntityIndex(entity_index);
            assert(index < chk.count && "destroying already destroyed entity! see version for more info.");
            return (T*)((uint8_t*)chk.memory + this->getOffset(type_id<T>().index))+ index;
        }
        ~Archetype(){
            this->destroy();
        }
    protected:
        size_t getOffset(typeid_t id)const {
            for(const auto& pair:this->components_index){
                if(pair.first == id)
                    return pair.second;
            }
            throw std::out_of_range("component id doesnot exist whitin this archtype");
        }
        // the entity chunk index
        inline entityIndex_t getChunkIndex(const entityIndex_t i) const {
            return (entityIndex_t)((size_t)i / this->chunk_capacity);
        }
        // index within a chunk
        inline entityIndex_t getEntityIndex(const entityIndex_t i) const {
            return (entityIndex_t)((size_t)i % this->chunk_capacity);
        }
    };
} // namespace DOTS


#endif // ARCHETYPE_HPP
