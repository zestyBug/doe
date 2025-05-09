#if !defined(REGISTER_HPP)
#define REGISTER_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <assert.h>
#include <memory>
#include "Archetype.hpp"
#include "System.hpp"
#include "cutil/bitset.hpp"
#include "cutil/SmallVector.hpp"
#include "cutil/span.hpp"

namespace DOTS
{
    // the class that holds all entities
    class Register final {
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t> entity_value{};

        std::vector<Archetype> archetypes{};
        // contains bitmask of archetype, or
        // may contain some buffering informations
        // if archetype is destroyed (in that case size/capacity is 0)
        std::vector<archtype_bitset> archetypes_id{};

        entityIndex_t free_entity_index = Entity::max_entity_count;
        archetypeIndex_t free_archetype_index = null_archetype_index;

        // find or create a archetype with given types,
        // NOTE: without initializing it
        // WARN: may create new archetype and may it requires resizing vector,
        // which leads to null current pointers.
        archetypeIndex_t getArchetypeIndex(const archtype_bitset& comp_bitmap){
            archetypeIndex_t archetype_index;

            for(archetype_index = 0;archetype_index < this->archetypes_id.size();archetype_index++){
                if(this->archetypes_id[archetype_index] == comp_bitmap && this->archetypes[archetype_index].capacity() != 0){
                    return archetype_index;
                }
            }

            if(this->free_archetype_index >= null_archetype_index){
                const auto i = this->archetypes_id.size();
                if(i>=null_archetype_index)
                    throw std::out_of_range("archtype index full");
                archetype_index = (archetypeIndex_t)i;
                this->archetypes_id.emplace_back();
                this->archetypes.emplace_back();
            }else{
                archetypeIndex_t next_index = this->archetypes_id[this->free_archetype_index].get<archetypeIndex_t>();
                /// TODO: may a invalid index check be good
                archetype_index = this->free_archetype_index;
                this->free_archetype_index = next_index;
            }
            this->archetypes_id[archetype_index] = comp_bitmap;
            this->archetypes[archetype_index].initialize(comp_bitmap);
            //printf("Debug: new archetype bitmask %u\n",comp_bitmap);
            return archetype_index;
        }
        // finds a empty index on entity array and return it index + version,
        // WARN: it may containts invalid value (not a real archetype index)
        Entity createEntity(){
            Entity result;
            if(free_entity_index < Entity::max_entity_count){
                entity_t& value = this->entity_value[free_entity_index];

                result = free_entity_index | (value.version << 24);
                free_entity_index = value.index;

                value.archetype = null_archetype_index;
            }else{
                const size_t index = entity_value.size();
                if(index >= Entity::max_entity_count)
                    throw std::out_of_range("register limit reached");
                entity_value.push_back(entity_t{});
                result = (Entity)index;
            }
            //printf("Debug: new entity %u\n",result);
            return result;
        }
        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const archetypeIndex_t archetype_index){
            if(this->archetypes[archetype_index].empty()) {
                //printf("Debug: destroy archetype bitmask %u\n",this->archetypes_id[archetype_index]);
                this->archetypes_id[archetype_index].set(free_archetype_index);
                free_archetype_index = archetype_index;
                this->archetypes[archetype_index].destroy();
            }
        }
        // same as destroyComponents() without calling destructor
        void destroyComponents2(entity_t value){
            const Entity modified = this->archetypes[value.archetype].destroyEntity(value.index,false);
            // if it index was filled with other entity
            if(modified.valid())
                this->entity_value[modified.index()].index = value.index;
            else
                // otherwise either archetype was empty or entity was indexed last one in array
                destroyEmptyArchetype(value.archetype);
        }
        /// remove an entity from an archetype
        // Warn: dont forget to set entity_t::index and entity_t::archetype to invalid values
        /// @param value entity value (index + archetype)
        void destroyComponents(entity_t value){
            const Entity modified = this->archetypes[value.archetype].destroyEntity(value.index);
            // if it index was filled with other entity
            if(modified.valid())
                this->entity_value[modified.index()].index = value.index;
            else
                // otherwise either archetype was empty or entity was indexed last one in array
                destroyEmptyArchetype(value.archetype);
        }

        template<typename Type>
        inline Type chunk_wrapper(const size_t chunk_index, const std::array<size_t,COMPOMEN_COUNT>& offsets, void*chunk){
            typeid_t id = type_id<Type>().index;
            return (Type*)(((char*)chunk) + offsets[type_id<Type>().index])[chunk_index];
        }

        template<typename Type>
        inline void init_wrapper(entityIndex_t index, const std::vector<std::pair<typeid_t,size_t>>& offsets, void*chunk, const Type& init_value){
            for(const auto& offset:offsets)
                if(offset.first == type_id<Type>().index)
                    new (((Type*)(((uint8_t*)chunk) + offset.second))+index) Type(init_value);
        }

    public:
        Register(){
            archetypes.reserve(8);
            entity_value.reserve(80);
        };
        ~Register(){};

        bool valid(Entity e) const {
            const Entity index = e.index();
            if(
                index >= this->entity_value.size() ||
                this->entity_value[index].version != e.version()
            ) return false;
            return true;
        }

        // create entity
        Entity create(const archtype_bitset& comp_bitmap = archtype_bitset{}) {
            const Entity result = createEntity();
            if(!comp_bitmap.all_zero()){
                const archetypeIndex_t archetype_index = this->getArchetypeIndex(comp_bitmap);
                this->entity_value[result.index()].index = this->archetypes[archetype_index].createEntity(result);
                this->entity_value[result.index()].archetype = archetype_index;
            }
            return result;
        }
        template<typename ... Args>
        Entity create() {
            const archtype_bitset comp_bitmap = componentsBitmask<Args...>();
            const Entity result = this->create(comp_bitmap);
            return result;
        }
        template<typename ... Args>
        Entity create(const Args& ... Argv) {
            const archtype_bitset comp_bitmap = componentsBitmask<Args...>();
            const Entity result = this->create(comp_bitmap);
            const entity_t value = this->entity_value[result.index()];
            auto& arch = this->archetypes[value.archetype];
            const size_t chunk_index = arch.getChunkIndex(value.index);
            const entityIndex_t in_chunk_index = arch.getEntityIndex(value.index);
            void*chunk  = arch.chunks[chunk_index].memory;
            (init_wrapper(in_chunk_index,arch.components_index,chunk,Argv) , ...);
            return result;
        }
        // destroy a entity entirly
        void destroy(Entity e) {
            if(!this->valid(e)){
                throw std::runtime_error("invalide entity");
                return;
            }
            const Entity index = e.index();
            entity_t& value = this->entity_value[index];

            // entity is empty
            if(value.valid())
                this->destroyComponents(value);

            value.index = free_entity_index;
            value.archetype = free_archetype_index;
            value.version++;
            free_entity_index = index;
        }

        void addComponents(Entity entity,const archtype_bitset& components_bitmap){
            if(!this->valid(entity)){
                throw std::runtime_error("invalide entity");
                return;
            }
            //printf("from: ");this->archetypes_id[value.archetype].debug();
            //printf("min:  ");components_bitmap.debug();
            //printf("equal:");to.debug();

            const Entity entity_index = entity.index();
            const entity_t old_value = this->entity_value.at(entity_index);
            entity_t new_value;

            // nothing to do?
            if(components_bitmap.all_zero())
                return;
            
            
            const archtype_bitset new_archetype = (old_value.archetype < null_archetype_index)?
                                                    this->archetypes_id.at(old_value.archetype) | components_bitmap
                                                    : components_bitmap ;
            // no change detected?
            if(old_value.archetype < null_archetype_index && 
                new_archetype == this->archetypes_id.at(old_value.archetype))
                return;
            
            new_value.archetype = this->getArchetypeIndex(new_archetype);

            auto &new_arch = this->archetypes.at(new_value.archetype);
            new_value.index = new_arch.createEntity(entity);
            new_value.version = old_value.version;
            this->entity_value.at(entity_index) = new_value;

            // new empty entity is just created
            if(old_value.archetype >= null_archetype_index)
                return;
            /*
             * buffers defined to improve cache frienldliness
             */

            auto &old_arch = this->archetypes.at(old_value.archetype);
            char *old_chunk = (char *) old_arch.chunks[old_arch.getChunkIndex(old_value.index)].memory;
            const size_t old_chunk_index = old_arch.getEntityIndex(old_value.index);
            const auto &old_index = old_arch.components_index;

            
            char *new_chunk = (char *) new_arch.chunks[new_arch.getChunkIndex(new_value.index)].memory;
            const size_t new_chunk_index = new_arch.getEntityIndex(new_value.index);
            // new archetype has more types than the old
            const auto &new_index = new_arch.components_index;

            if(old_index.size() >= new_index.size())
                throw std::runtime_error("archtype being increased to smaller archtype");

            typeid_t i=0,j=0;
            for (;j < new_index.size() && i < old_index.size();j++){
                // buffer
                const typeid_t old_type = old_index.at(i).first;
                // buffer
                const typeid_t new_type = new_index.at(j).first;

                const comp_info component_info = type_id(new_type);
                void * const src = old_chunk + old_index.at(i).second + old_chunk_index * component_info.size;
                void *       dst = new_chunk + new_index.at(j).second + new_chunk_index * component_info.size;
                
                if(new_type == old_type){
                    memcpy(dst,src,component_info.size);
                    i++;
                    continue;
                }
                if(new_type > old_type)
                    throw std::runtime_error("a component does not exist in new archetype");
            }
            destroyComponents2(old_value);
        }

        void removeComponents(Entity entity,const archtype_bitset& components_bitmap){
            if(!this->valid(entity)){
                throw std::runtime_error("invalide entity");
                return;
            }
            //printf("from: ");this->archetypes_id[value.archetype].debug();
            //printf("min:  ");components_bitmap.debug();
            //printf("equal:");to.debug();

            const Entity entity_index = entity.index();
            const entity_t old_value = this->entity_value.at(entity_index);

            //empty entities contains invalid archetype id
            if(old_value.archetype < null_archetype_index)
            {
                if(this->archetypes_id.at(old_value.archetype).all_zero())
                    throw std::runtime_error("invalid archetype");
                entity_t new_value = entity_t{};

                {
                    archtype_bitset new_archetype = this->archetypes_id.at(old_value.archetype).and_not(components_bitmap);
                    if(new_archetype.all_zero()){
                        destroyComponents(old_value);
                        this->entity_value.at(entity_index).archetype = null_archetype_index;
                        return;
                    }else
                        new_value.archetype = this->getArchetypeIndex(new_archetype);
                }
                /*
                 * buffers defined to improve cache frienldliness
                 */

                auto &old_arch = this->archetypes.at(old_value.archetype);
                char *old_chunk = (char *) old_arch.chunks[old_arch.getChunkIndex(old_value.index)].memory;
                const size_t old_chunk_index = old_arch.getEntityIndex(old_value.index);
                const auto &old_index = old_arch.components_index;

                auto &new_arch = this->archetypes.at(new_value.archetype);
                {
                    new_value.index = new_arch.createEntity(entity);
                    new_value.version = old_value.version;
                    this->entity_value.at(entity_index) = new_value;
                }
                char *new_chunk = (char *) new_arch.chunks[new_arch.getChunkIndex(new_value.index)].memory;
                const size_t new_chunk_index = new_arch.getEntityIndex(new_value.index);
                // new archetype has less types than the old
                const auto &new_index = new_arch.components_index;

                if(old_index.size() <= new_index.size())
                    throw std::runtime_error("archtype being decreased to larger archtype");

                typeid_t i=0,j=0;
                for (;i < old_index.size() && j < new_index.size();i++){
                    // buffer
                    const typeid_t old_type = old_index.at(i).first;
                    // buffer
                    const typeid_t new_type = new_index.at(j).first;

                    const comp_info component_info = type_id(old_type);
                    void * const src = old_chunk + old_index.at(i).second + old_chunk_index * component_info.size;
                    
                    if(new_type == old_type){
                        void *   dst = new_chunk + new_index.at(j).second + new_chunk_index * component_info.size;
                        memcpy(dst,src,component_info.size);
                        j++;
                        continue;
                    }
                    // a component did not exist in old archetype
                    if(new_type < old_type)
                        throw std::runtime_error("archtype being decreased to larger archtype");
                    // a component does not exist in new archetype
                    if(component_info.destructor)
                        component_info.destructor(src);
                }
                for (;i < old_index.size();i++){
                    const typeid_t old_type = old_index.at(i).first;
                    const comp_info component_info = type_id(old_type);
                    void * const src = old_chunk + old_index.at(i).second + old_chunk_index * component_info.size;
                    if(component_info.destructor)
                        component_info.destructor(src);
                }
                destroyComponents2(old_value);
            }
        }
        template<typename T>
        T& getComponent(Entity e) const {
            if(!this->valid(e)) return *(T*)nullptr;
            const Entity index = e.index();
            const entity_t value = this->entity_value[index];
            // empty entity
            value.validate();
            // component does not exists
            if(this->archetypes_id.at(value.archetype)[type_id<T>().index] == false)
                throw std::out_of_range("Component does not exist");
            auto& arch = this->archetypes.at(value.archetype);
            char * const chunk = (char *) arch.chunks.at(arch.getChunkIndex(value.index)).memory;
            return (T*)(chunk + arch.getOffset(type_id<T>().index))[value.index];
        }

        // optimized: using archetype bitmap
        template<typename T>
        bool hasComponent(Entity e) const {
            if(!this->valid(e))
                return false;
            const Entity index = e.index();
            const archetypeIndex_t arch_index = this->entity_value.at(index).archetype;
            if(arch_index >= null_archetype_index)
                throw std::runtime_error("bad entity");
            return this->archetypes_id.at(arch_index)[type_id<T>().index];
        }
        // optimized: using archetype bitmap
        bool hasComponents(const Entity e,const archtype_bitset& components_bitmap) const {
            if(!this->valid(e)) return false;
            const Entity index = e.index();
            const entity_t value = this->entity_value.at(index);
            // no component
            if(!value.valid())
                return false;
            return (this->archetypes_id.at(value.archetype) & components_bitmap) == components_bitmap;
        }

        // TODO: recives a chunk size, so gives call the callback multiple times
        // with array of pointers to components plus entity id list and maximum size of arrays as arguments
        template<typename ... Types>
        void iterate(void(*func)(std::array<void*, sizeof...(Types)>,Entity*,size_t)) {
            const archtype_bitset comps_bitmap = componentsBitmask<Types...>();
            const typeid_t comps_index[sizeof...(Types)] = {(type_id<Types>().index)...};
            std::array<size_t, sizeof...(Types)> offset_buffer;
            std::array<void*, sizeof...(Types)> arg_buffer;

            for (archetypeIndex_t i = 0; i < this->archetypes_id.size(); i++)
                // iterate through archetypes_id then check archetypes::size results in less cache miss
                if(comps_bitmap.and_equal(this->archetypes_id[i])){
                    Archetype &arch = this->archetypes[i];
                    if(arch.capacity() == 0)
                        continue;
                    span<Archetype::chunk> chunks{arch.chunks.data(),arch.chunks.size()};

                    // fill offset_buffer
                    for(size_t j=0;j<offset_buffer.size();j++){
                        for(const auto& com:arch.components_index)
                            if(com.first == comps_index[j]){
                                offset_buffer[j] = com.second;
                                goto JMPING;
                            }
                        throw std::runtime_error("failed to find a component offset");
                        JMPING:;
                    }

                    for(size_t chunck_index=0; chunck_index < chunks.size(); chunck_index++) {
                        auto& chunk = chunks.at(chunck_index);
                        if(chunk.count == 0)
                            throw std::runtime_error("found an empty chunk");
                        for (size_t index = 0; index < arg_buffer.size(); index++)
                            arg_buffer[index] = ((char*)chunk.memory) + offset_buffer[index];
                        func( arg_buffer,&arch.entities_id.at(chunck_index*arch.chunk_capacity),chunk.count);
                    }
                }
        }
        Archetype& getArchetype(const entity_t value){
             return this->archetypes[value.archetype];
        }
        const Archetype& getArchetype(const entity_t value) const {
             return this->archetypes[value.archetype];
        }
    };

} // namespace DOTS


#endif // REGISTER_HPP
