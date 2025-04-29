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
        // if archtype is destroyed (in that case size/capacity is 0)
        std::vector<archtype_bitset> archetypes_id{};

        entityIndex_t free_entity_index = Entity::max_entity_count;
        archetypeIndex_t free_archetype_index = null_archetype_index;

        // find or create a archetype with given types,
        // NOTE: without initializing it
        // WARN: may create new archtype and may it requires resizing vector,
        // which leads to null current pointers.
        archetypeIndex_t getArchetypeIndex(const archtype_bitset& comp_bitmap){
            archetypeIndex_t archetype_index;

            for(archetype_index = 0;archetype_index < this->archetypes_id.size();archetype_index++){
                if(this->archetypes_id[archetype_index] == comp_bitmap && this->archetypes[archetype_index].capacity() != 0){
                    return archetype_index;
                }
            }

            if(this->free_archetype_index >= null_archetype_index){
                archetype_index = this->archetypes_id.size();
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
            if(free_entity_index != Entity::max_entity_count){
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
        /// remove an entity from an archtype
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
        // charges archetype, recives entity id and components bitmap
        // also updates entity_value
        void _changeComponent(Entity entity, const archtype_bitset& new_archetype){
            if(!this->valid(entity)) return;
            archtype_bitset *old_archetype = nullptr;
            const Entity entity_index = entity.index();
            const entity_t old_value = this->entity_value[entity_index];

            //empty entities contains invalid archetype id
            if(old_value.archetype < null_archetype_index){
                /* DEBUG: */
                assert(!this->archetypes_id.at(old_value.archetype).all_zero());
                if(new_archetype.all_zero()){
                    destroyComponents(old_value);
                    this->entity_value[entity_index].archetype = null_archetype_index;
                    return;
                }
                // else
            }else{
                // no change
                if(new_archetype.all_zero())
                    return;
                // else
            }

            
            
            entity_t new_value = entity_t{};
            new_value.archetype = this->getArchetypeIndex(new_archetype);
            new_value.index = this->archetypes[new_value.archetype].createEntity(entity);
            this->entity_value[entity_index] = new_value;

            // warn: archetypes_id may reallocate after calling getArchetypeIndex!
            old_archetype = & this->archetypes_id.at(old_value.archetype);
            

            // has no older form?
            if(old_archetype == nullptr)
                return;

            auto &new_arch = this->archetypes[new_value.archetype];
            char *new_chunk = (char *) new_arch.chunks[new_arch.getChunkIndex(new_value.index)].memory;
            const size_t new_chunk_index = new_arch.getEntityIndex(new_value.index);
            const auto &new_offsets = new_arch.component_offset;


            auto &old_arch = this->archetypes[old_value.archetype];
            char *old_chunk = (char *) old_arch.chunks[old_arch.getChunkIndex(old_value.index)].memory;
            const size_t old_chunk_index = new_arch.getEntityIndex(new_value.index);
            const auto &old_offsets = old_arch.component_offset;

            const archtype_bitset shared_com = new_archetype & *old_archetype;



            for (typeid_t i = 0; i < shared_com.capacity(); i++){
                if(shared_com[i]) {
                    const comp_info component_info = type_id(i);


                    // TODO: does *_component_bitmap means we can access component array blindly? i mean without null pointer check.
                    void  * const src = old_chunk + old_offsets[i] + old_chunk_index * component_info.size;
                    //((char*)(this->archetypes[old_value.archetype].components[i].pointer)) + (old_value.index * component_info.size);
                    if(new_archetype[i]){
                        void *dst = new_chunk + new_offsets[i] + new_chunk_index * component_info.size;
                        //((char*)(this->archetypes[new_value.archetype].components[i].pointer)) + (new_value.index * component_info.size);
                        memcpy(dst,src,component_info.size);
                    }else{
                        if(component_info.destructor)
                            component_info.destructor(src);
                    }
                }
            }
            destroyComponents2(old_value);
        }

        template<typename Type>
        inline Type chunk_wrapper(const size_t chunk_index, const std::array<size_t,COMPOMEN_COUNT>& offsets, void*chunk){
            typeid_t id = type_id<Type>().index;
            return (Type*)(((char*)chunk) + offsets[type_id<Type>().index])[chunk_index];
        }

        template<typename Type>
        inline void init_wrapper(entityIndex_t index, const std::array<size_t,COMPOMEN_COUNT>& offsets, void*chunk, const Type& init_value){
            new  (((Type*)(((uint8_t*)chunk) + offsets[type_id<Type>().index]))+index) Type(init_value);
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
        Entity create(const Args& ... Argv) {
            const archtype_bitset comp_bitmap = componentsBitmask<Args...>();
            const Entity result = this->create(comp_bitmap);
            const entity_t value = this->entity_value[result.index()];
            auto& arch = this->archetypes[value.archetype];
            const size_t chunk_index = arch.getChunkIndex(value.index);
            const entityIndex_t in_chunk_index = arch.getEntityIndex(value.index);
            void*chunk  = arch.chunks[chunk_index].memory;
            (init_wrapper(in_chunk_index,arch.component_offset,chunk,Argv) , ...);
            return result;
        }
        // destroy a entity entirly
        void destroy(Entity e) {
            if(!this->valid(e)) return;
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

        void addComponents(Entity e,const archtype_bitset& components_bitmap){
            if(!this->valid(e)) return;
            const Entity index = e.index();
            entity_t& value = this->entity_value[index];

            //compid_t components_bitmap = componentsBitmask<T>();

            // empty entities contains invalid component id
            if(value.archetype >= null_archetype_index)
            {
                _changeComponent(e,components_bitmap);
            }else{
                _changeComponent(e,components_bitmap | this->archetypes_id[value.archetype]);
            }
        }

        void removeComponents(Entity e,const archtype_bitset& components_bitmap){
            if(!this->valid(e)) return;
            const Entity index = e.index();
            entity_t& value = this->entity_value[index];
            archtype_bitset to = this->archetypes_id[value.archetype].and_not(components_bitmap);
            printf("from: ");this->archetypes_id[value.archetype].debug();
            printf("min:  ");components_bitmap.debug();
            printf("equal:");to.debug();
            if(value.archetype < null_archetype_index)
                _changeComponent(e,to);
        }
        template<typename T>
        T& getComponent(Entity e) const {
            if(!this->valid(e)) return *(T*)nullptr;
            const Entity index = e.index();
            const entity_t value = this->entity_value[index];

            // empty entity
            value.validate();
            // component does not exists
            if(this->archetypes_id[value.archetype][type_id<T>().index] == false)
                throw std::range_error("Component does not exist");
            auto& arch = this->archetypes[value.archetype];
            char * const chunk = (char *) arch.chunks[arch.getChunkIndex(value.index)].memory;
            return (T*)(chunk + arch.component_offset[type_id<T>().index])[value.index];
        }
        template<typename T>
        bool hasComponent(Entity e) const {
            if(!this->valid(e)) return false;
            const Entity index = e.index();
            const archetypeIndex_t arch_index = this->entity_value[index].archetype;

            if(arch_index >= null_archetype_index)
                throw std::runtime_error("bad entity");
            return this->archetypes_id[arch_index][type_id<T>().index];
        }
        bool hasComponents(const Entity e,const archtype_bitset& components_bitmap) const {
            if(!this->valid(e)) return false;
            const Entity index = e.index();
            const entity_t value = this->entity_value[index];

            // no component
            if(!value.valid())
                return false;
            return (this->archetypes_id[value.archetype] & components_bitmap) == components_bitmap;
        }
        // TODO: recives a chunk size, so gives call the callback multiple times
        // with array of pointers to components plus entity id list and maximum size of arrays as arguments
        template<typename ... Types>
        void iterate(void(*func)(std::array<void*, sizeof...(Types)>,Entity*,size_t)) {
            const archtype_bitset comps_bitmap = componentsBitmask<Types...>();
            const typeid_t comps_index[sizeof...(Types)] = {(type_id<Types>().index)...};
            for (archetypeIndex_t i = 0; i < this->archetypes_id.size(); i++)
                // iterate through archetypes_id then check archetypes::size results in less cache miss
                if(comps_bitmap.and_equal(this->archetypes_id[i])){
                    Archetype &arch = this->archetypes[i];
                    if(arch.capacity() == 0)
                        continue;
                    span<Archetype::chunk> chunks{arch.chunks.data(),arch.chunks.size()};
                    for(size_t chunck_index=0; chunck_index < chunks.size(); chunck_index++) {
                            if(chunks.at(chunck_index).count == 0)
                                continue;
                            std::array<void*, sizeof...(Types)> args;
                            for (size_t comp_index = 0; comp_index < sizeof...(Types); comp_index++)
                                args[comp_index] = ((char*)chunks.at(chunck_index).memory) + arch.component_offset[comps_index[comp_index]];
                            func( args,&arch.entities_id.at(chunck_index*arch.chunk_capacity),chunks.at(chunck_index).count);

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
