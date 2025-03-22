#if !defined(REGISTER_HPP)
#define REGISTER_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <assert.h>
#include <memory>
#include "Archtype.hpp"
#include "System.hpp"
#include "cutil/bitset.hpp"
#include "cutil/SmallVector.hpp"

namespace DOTS
{
    // the class that holds all entities
    class Register final {
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t> entity_value;

        std::vector<Archetype> archetypes;
        // contains bitmask of archetype,
        // contains something else value if archetype
        // is not allocated (and size/capacity is set to 0)
        std::vector<bitset> archetypes_id;

        entityId_t free_entity_index = Entity::max_entity_count;
        archetypeId_t free_archetype_index = null_archetype_index;

        // find or create a archetype with given types,
        // NOTE: without initializing it
        archetypeId_t getArchetypeIndex(const bitset& comp_bitmap){
            archetypeId_t archetype_index;

            for(size_t i = 0;i < this->archetypes_id.size();i++){
                if(this->archetypes_id[i] == comp_bitmap && this->archetypes[i].capacity != 0){
                    return i;
                }
            }

            if(this->free_archetype_index >= null_archetype_index){
                archetype_index = this->archetypes_id.size();
                this->archetypes_id.emplace_back();
                this->archetypes.emplace_back();
            }else{
                archetypeId_t next_index = this->archetypes_id[this->free_archetype_index].get<archetypeId_t>();
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
                assert(index < Entity::max_entity_count && "register limit reached");
                entity_value.push_back(entity_t{});
                result = (Entity)index;
            }
            //printf("Debug: new entity %u\n",result);
            return result;
        }
        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const archetypeId_t archetype_index){
            if(this->archetypes[archetype_index].size == 0) {
                //printf("Debug: destroy archetype bitmask %u\n",this->archetypes_id[archetype_index]);
                this->archetypes_id[archetype_index].set(free_archetype_index);
                free_archetype_index = archetype_index;
                this->archetypes[archetype_index].destroy();
            }
        }
        // same as destroyComponents() without calling destructor
        void destroyComponents2(entity_t value){
            const Entity modified = this->archetypes[value.archetype].destroy2(value.index);
            // if it index was filled with other entity
            if(modified.valid())
                this->entity_value[modified.index()].index = value.index;
            else
                // otherwise either archetype was empty or entity was indexed last one in array
                destroyEmptyArchetype(value.archetype);
        }
        /// remove all comonents from an entity
        /// @param value entity value (index + archetype)
        void destroyComponents(entity_t value){
            const Entity modified = this->archetypes[value.archetype].destroy_entity(value.index);
            // if it index was filled with other entity
            if(modified.valid())
                this->entity_value[modified.index()].index = value.index;
            else
                // otherwise either archetype was empty or entity was indexed last one in array
                destroyEmptyArchetype(value.archetype);
        }
        // charges archetype, recives entity id and components bitmap
        // also updates entity_value
        void _changeComponent(Entity entity, const bitset& new_archetype){
            if(!this->valid(entity)) return;
            bitset *old_archetype = nullptr;
            const Entity entity_index = entity.index();
            entity_t old_value = this->entity_value[entity_index];

            //empty entities contains invalid archetype id
            if(old_value.archetype < null_archetype_index){
                /* DEBUG: */
                assert(!this->archetypes_id[old_value.archetype].all_zero());
                if(new_archetype.all_zero()){
                    destroyComponents(old_value);
                    this->entity_value[entity_index].archetype = null_archetype_index;
                    return;
                }
                // else
            }else{
                if(new_archetype.all_zero())
                    return;
                // else
            }

            entity_t new_value = entity_t{};
            new_value.archetype = this->getArchetypeIndex(new_archetype);
            new_value.index = this->archetypes[new_value.archetype].allocate_entity(entity);
            this->entity_value[entity_index] = new_value;

            if(old_archetype == nullptr)
                return;

            // moving old components
            size_t max_size = std::max(old_archetype->capacity(),new_archetype.capacity());
            for (typeid_t i = 0; i < max_size; i++){
                if((*old_archetype)[i]) {
                    const comp_info component_info = type_id(i);
                    // TODO: does *_component_bitmap means we can access component array blindly? i mean without null pointer check.
                    void *src = ((char*)(this->archetypes[old_value.archetype].components[i].pointer)) + (old_value.index * component_info.size);
                    if(new_archetype[i]){
                        void *dst = ((char*)(this->archetypes[new_value.archetype].components[i].pointer)) + (new_value.index * component_info.size);
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
        inline Type template_wrapper(entity_t value){
            Archetype *arch = &(this->archetypes[value.archetype]);
            typeid_t id = type_id<Type>().index;
            Archetype::single_component *com = &(arch->components[id]);
            void *ptr = com->pointer;
              return ((std::add_pointer_t<std::remove_const_t<std::remove_reference_t<Type>>>)ptr) [value.index];
            //return ((std::add_pointer_t<std::remove_const_t<std::remove_reference_t<Type>>>)(this->archetypes[value.archetype].components[type_id<Type>().index].pointer)) [value.index];
        }
        template<typename Type>
        inline void init_wrapper(entity_t value,const Type& init_value){
            ((Type*)this->archetypes[value.archetype].components[type_id<Type>().index].pointer) [value.index] = init_value;
        }

    public:
        Register(){};
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
        Entity create(const bitset& comp_bitmap = bitset{}) {
            const Entity result = createEntity();
            if(!comp_bitmap.all_zero()){
                const archetypeId_t archetype_index = this->getArchetypeIndex(comp_bitmap);
                this->entity_value[result.index()].index = this->archetypes[archetype_index].allocate_entity(result);
                this->entity_value[result.index()].archetype = archetype_index;
            }
            return result;
        }
        template<typename ... Args>
        Entity create(const Args& ... Argv) {
            const bitset comp_bitmap = componentsBitmask<Args...>();
            const Entity result = this->create(comp_bitmap);
            const entity_t value = this->entity_value[result.index()];
            (init_wrapper(value, Argv) , ...);
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

        void addComponents(Entity e,const bitset& components_bitmap){
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

        void removeComponents(Entity e,const bitset& components_bitmap){
            if(!this->valid(e)) return;
            const Entity index = e.index();
            entity_t& value = this->entity_value[index];
            bitset to = this->archetypes_id[value.archetype].and_not(components_bitmap);
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
                  assert(this->archetypes[value.archetype].components[type_id<T>().index].pointer);
            return ((T*)(this->archetypes[value.archetype].components[type_id<T>().index].pointer))[value.index];
        }
        template<typename T>
        bool hasComponent(Entity e) const {
            if(!this->valid(e)) return false;
            const Entity index = e.index();
            const entity_t value = this->entity_value[index];

            if(value.archetype < null_archetype_index){
                const auto& vec = this->archetypes[value.archetype].components;
                if(vec.size() > type_id<T>().index)
                    if(vec[type_id<T>().index].pointer)
                        return true;
            }
            return false;
        }
        bool hasComponents(const Entity e,const bitset& components_bitmap) const {
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
        void iterate(void(*func)(std::array<void*, sizeof...(Types) + 1>,size_t), const size_t chunk_size = 16) {
            const bitset   comps_bitmap = componentsBitmask<Types...>();
            const typeid_t comps_index[sizeof...(Types)] = {(type_id<Types>().index)...};
            const size_t   comps_size[sizeof...(Types)] = {(type_id<Types>().size)...};
            std::array<void*, sizeof...(Types) + 1> args;
            for (archetypeId_t i = 0; i < this->archetypes_id.size(); i++)
                // iterate through archetypes_id then check archetypes::size results in less cache miss
                if(comps_bitmap.and_equal(this->archetypes_id[i])){
                    Archetype &arch = this->archetypes[i];
                    if(arch.size != 0)
                        for(size_t chunck_index=0; chunck_index < arch.size; chunck_index+=chunk_size) {
                            size_t comp_index = 0;
                            for (; comp_index < sizeof...(Types); comp_index++)
                                args[comp_index] = ((char*)(arch.components[comps_index[comp_index]].pointer)) + comps_size[comp_index] * chunck_index;
                            args[comp_index] = arch.entity_id + chunck_index;
                            const size_t remaind = arch.size - chunck_index;
                            func(args,std::min(remaind,chunk_size));
                        }
                }
        }

        // TODO: recives a function and template
        template<typename ... Types>
        void iterate(void(*func)(Entity,Types...)) {
            const bitset comps_bitmap = componentsBitmask<Types...>();
            printf("comps_bitmap: ");
            comps_bitmap.debug();
            for (archetypeId_t i = 0; i < this->archetypes_id.size(); i++){
                // arch might be empty and unallocatted
                const Archetype &arch = this->archetypes[i];
                if(arch.capacity == 0)
                    continue;
                printf("archetypes_id: ");
                this->archetypes_id[i].debug();
                if(comps_bitmap.and_equal(this->archetypes_id[i])){
                    for(size_t entity_index=0; entity_index < std::min(arch.size,(size_t)Entity::max_entity_count); entity_index++)
                        func( arch.entity_id[entity_index], template_wrapper<Types>(entity_t{.index=(entityId_t)entity_index,.archetype=i,.version=0}) ...);
                }
            }
        }
        /// @brief A helper function for job systems, searchs one archetype after another archetype
        /// @tparam ...Types components that must be included
        /// @param from begining entity value to start searching, pass 0 if want to begin from zero
        /// @param chunck_size maximum number of entities to pick in one single entity range
        /// @return range of available entities in value in same archetype exluding the end one, if begin and end was same
        // means it is an empty range and we found nothing.
        template<typename ... Types>
        entity_range findChunk(entity_t from, const size_t chunck_size) const {
            const bitset comps_bitmap = componentsBitmask<Types...>();
            for (archetypeId_t i = from.archetype; i < this->archetypes_id.size(); i++)
            {
                if(comps_bitmap.and_equal(this->archetypes_id[i]))
                    if(from.index < this->archetypes[i].size)
                        return entity_range{
                            .begin=from.index,
                            .end=std::min<entityId_t>(from.index+chunck_size, this->archetypes[i].size),
                            .archetype=i
                        };
                from.index = 0;
            }
            return entity_range{.begin=from.index, .end=from.index, .archetype=null_archetype_index};
        }

        template<typename T>
        const T* getComponent2(const entity_t value) const {
            // empty entity
            value.validate();
            void * const ptr = this->archetypes[value.archetype].components[type_id<T>().index].pointer;
            // component does not exists
            assert(ptr);
            return (const T*)ptr + value.index;
        }
        template<typename T>
        T* getComponent2(const entity_t value) {
            // empty entity
            value.validate();
            void * const ptr = this->archetypes[value.archetype].components[type_id<T>().index].pointer;
            // component does not exists
            assert(ptr);
            return (T*)ptr + value.index;
        }
        Archetype& getArchetype2(const entity_t value){
             return this->archetypes[value.archetype];
        }
        const Archetype& getArchetype2(const entity_t value) const {
             return this->archetypes[value.archetype];
        }
    };

} // namespace DOTS


#endif // REGISTER_HPP
