#include <stdint.h>
#include <vector>
#include <array>
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
                printf("New memorypool at: %p\n",this->memory);
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
                    printf("free(%p)\n",this->memory);
                    free(this->memory);
                    this->memory = nullptr;
                }
            }
        };
        struct component_info {
            comp_info info{};
            size_t offset = 0;
        };
        friend class Register;
        static constexpr size_t CHUNK_SIZE = 4096;
        // invalid index and max entity number
        static constexpr entityIndex_t INVALID_INDEX = 0xffffff;
        // maximum number of entities that can be fit into a single chunk
        size_t chunk_capacity = 0;
        // note: chunks are united, means
        // if we remove a entity for a random chunk,
        // last entity on lastg chunk will be
        // filled in it place so iterating more efficiently.
        std::vector<chunk> chunks{};
        // list of information about every component that exist in chunks
        // index is sorted as same sort as components in chunks
        std::vector<component_info> components_info{};
        std::vector<Entity> entities_id{};
        // bit index ==> component offset
        // first check if component extst using components_info,
        // may contain ivalid number if component does not exist.
        std::array<size_t,COMPOMEN_COUNT> component_offset{};
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
                this->components_info.reserve(component_count);
                //this->component_offset.resize(last_bit);
                this->entities_id.reserve(8);
            }
            {
                size_t size_sum = 0;
                for(size_t i=0;i < types.capacity();i++)
                    if(types[i]){
                        component_info cinfo;
                        cinfo.info=rtti[i];
                        cinfo.offset=size_sum*this->chunk_capacity;
                        size_sum += cinfo.info.size;
                        this->components_info.emplace_back(cinfo);
                        component_offset.at(i) = cinfo.offset;
                    }
            }
        }



        void destroy(){
            for(chunk &ck:chunks)
            {
                for(const auto& c_info:this->components_info){
                    const auto info = c_info.info;
                    uint8_t *mem = (uint8_t*)ck.memory;
                    const uint8_t *dest = (uint8_t *)mem + ck.count * info.size;
                    for(;mem<dest;mem+=info.size)
                        info.destructor(mem);
                }

                ck.count = 0;
            }
            this->chunk_capacity = 0;
            this->chunks = std::vector<chunk>();
            this->components_info = std::vector<component_info>();
            //this->component_offset = std::vector<size_t>();
            this->entities_id = std::vector<Entity>();
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
        Entity destroyEntity(entityIndex_t entity_index, bool destruction = true){
            assert(entity_index < this->entities_id.size());
            assert(0 < this->chunks.size());


            entityIndex_t in_chunk_index;
            uint8_t* old_chk_memory;
            {
                chunk& old_chk = this->chunks.at(getChunkIndex(entity_index));
                in_chunk_index = getEntityIndex(entity_index);
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
                for(size_t i=0;i<this->components_info.size();i++)
                {
                    const component_info info = this->components_info.at(i);
                    uint8_t * const ptr1 = old_chk_memory + info.offset + (info.info.size * in_chunk_index);
                    if(info.info.destructor)
                        info.info.destructor(ptr1);
                }


            Entity last_entity = this->entities_id.back();
            this->entities_id.pop_back();

            // if last entity is the deleted entity
            if(last_chk_memory == old_chk_memory && last_in_chunk_index == in_chunk_index){
                last_entity = INVALID_INDEX;
                goto END;
            }

            for(size_t i=0;i<this->components_info.size();i++){
                const component_info info = this->components_info.at(i);
                      uint8_t *ptr1 = old_chk_memory  + info.offset + (info.info.size*     in_chunk_index);
                const uint8_t *ptr2 = last_chk_memory + info.offset + (info.info.size*last_in_chunk_index);
                memcpy(ptr1,ptr2,info.info.size);
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
            return (uint8_t*)chk.memory + this->component_offset.at(component_id) + rtti[component_id].size*index;
        }
        // requesting component that does not exist results undefined behaviour
        template<typename T>
        T* getComponent(entityIndex_t entity_index){
            assert(entity_index < this->entities_id.size());
            chunk& chk = this->chunks.at(getChunkIndex(entity_index));
            const entityIndex_t index = getEntityIndex(entity_index);
            assert(index < chk.count && "destroying already destroyed entity! see version for more info.");
            return (T*)((uint8_t*)chk.memory + this->component_offset.at(type_id<T>().index))+ index;
        }
        ~Archetype(){
            this->destroy();
        }
    protected:
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
