#if !defined(REGISTER_HPP)
#define REGISTER_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <assert.h>
#include <memory>
#include "Archtype.hpp"
#include "System.hpp"

namespace DOTS
{
    
    class Register final {
        // array of entities value,
        // contains index of it archtype and it index in that archtype 
        std::vector<entity_t> entity_value;
        std::array<Archtype,256> archtypes;
        // contains bitmask of archtype, 
        // contains something else if archtype is not allocated and size/capacity is set to 0
        std::array<compid_t,256> archtypes_id;
        // contains index of last archtype, acts as size of archtypes_id and archtypes array
        archtypeId_t archtypes_index = 0;
        std::vector<std::unique_ptr<System>> system;

        Entity free_entity_index = null_entity_index;
        archtypeId_t free_archtype_index = null_archtype_index;

        // find or create a archtype with given types, 
        // NOTE: without initializing it
        archtypeId_t getArchtypeIndex(compid_t comp_bitmap){
            archtypeId_t archtype_index;

            for(archtypeId_t i = 0;i < this->archtypes_index;i++){
                if(this->archtypes_id[i] == comp_bitmap && this->archtypes[i].capacity != 0){
                    return i;
                }
            }

            if(free_archtype_index == null_archtype_index){
                assert(this->archtypes_index < this->archtypes_id.size());
                archtype_index = this->archtypes_index;
                this->archtypes_index++;
            }else{
                archtypeId_t next_index = this->archtypes_id[this->free_archtype_index];
                /// TODO: may a invalid index check be good
                archtype_index = this->free_archtype_index;
                this->free_archtype_index = next_index;
            }
            this->archtypes_id[archtype_index] = comp_bitmap;
            this->archtypes[archtype_index].initialize(comp_bitmap);
            //printf("Debug: new archtype bitmask %u\n",comp_bitmap);
            return archtype_index;
        }
        // finds a empty index on entity array and return it index + version, 
        // WARN: it may containts invalid value
        Entity createEntity(){
            Entity result;
            if(free_entity_index < null_entity){
                entity_t& value = this->entity_value[free_entity_index];
                value.version++;

                result = free_entity_index | (value.version << 24);
                free_entity_index = value.index;

                value.index = null_entity_index;
                value.archtype = null_entity;
            }else{
                const size_t index = entity_value.size();
                assert(index < null_entity);
                entity_value.push_back(entity_t{});
                result = index;
            }
            //printf("Debug: new entity %u\n",result);
            return result;
        }
        // destroy component if empty otherwise nothing
        void destroyEmptyComponent(const archtypeId_t archtype_index){
            if(this->archtypes[archtype_index].size == 0) {
                //printf("Debug: destroy archtype bitmask %u\n",this->archtypes_id[archtype_index]);
                this->archtypes_id[archtype_index] = free_archtype_index;
                free_archtype_index = archtype_index;
                this->archtypes[archtype_index].destroy();
            }
        }
        // same as destroyComponents() without calling destructor
        void destroyComponents2(entity_t value){
            const Entity modified = get_index( this->archtypes[value.archtype].destroy2(value.index) );
            // if it index was filled with other entity
            if(modified != null_entity)
                this->entity_value[get_index(modified)].index = value.index;
            else
                // otherwise either archtype was empty or entity was indexed last one in array
                destroyEmptyComponent(value.archtype);
        }
        // removes comonents not entity itself, recives entity value (index + archtype)
        void destroyComponents(entity_t value){
            const Entity modified = this->archtypes[value.archtype].destroy(value.index);
            // if it index was filled with other entity
            if(modified != null_entity)
                this->entity_value[get_index(modified)].index = value.index;
            else
                // otherwise either archtype was empty or entity was indexed last one in array
                destroyEmptyComponent(value.archtype);
        }
        // charges archtype, recives entity id and components bitmap
        // also updates entity_value
        void _changeComponent(Entity entity, compid_t new_component_bitmap){
            if(!this->valid(entity)) return;
            compid_t old_component_bitmap;
            const Entity entity_index = get_index(entity);
            entity_t old_value = this->entity_value[entity_index];

            //empty entities contains invalid archtype id
            if(old_value.index == null_entity_index){
                old_component_bitmap = 0;
            }else{
                old_component_bitmap = this->archtypes_id[old_value.archtype];
            }
            
            if(old_component_bitmap == new_component_bitmap){// nothing to be done
            }else if(new_component_bitmap == 0){// change to no component
                if(old_component_bitmap != 0){
                    // remove all components
                    destroyComponents(old_value);
                }
                this->entity_value[entity_index].index = null_entity_index;
                this->entity_value[entity_index].archtype = null_archtype_index;
            }else{
                // a buffer in stack
                entity_t new_value = entity_t{};
                new_value.archtype = this->getArchtypeIndex(new_component_bitmap);
                new_value.index = this->archtypes[new_value.archtype].allocate(entity);
                this->entity_value[entity_index] = new_value;
                // literally creates a new entity
                if(old_component_bitmap == 0){
                }else{ // moving old components
                    for (compid_t i = 0; i < 32; i++){
                        if(old_component_bitmap & 1) {
                            const comp_info component_info = type_id(i);
                            // TODO: does *_component_bitmap means we can access component array blindly? i mean without null pointer check.
                            void *src = ((char*)(this->archtypes[old_value.archtype].components[i])) + (old_value.index * component_info.size);
                            if(new_component_bitmap & 1){
                                void *dst = ((char*)(this->archtypes[new_value.archtype].components[i])) + (new_value.index * component_info.size);
                                memcpy(dst,src,component_info.size);
                            }else{
                                if(component_info.destructor)
                                    component_info.destructor(src);
                            }
                        }
                        old_component_bitmap >>= 1;
                        new_component_bitmap >>= 1;
                    }
                    destroyComponents2(old_value);
                }
            }
        }

        template<typename Type>
        inline Type template_wrapper(entity_t value){
            return ((std::add_pointer_t<std::remove_const_t<std::remove_reference_t<Type>>>)this->archtypes[value.archtype].components[type_id<Type>().index]) [value.index];
        }
        template<typename Type>
        inline void init_wrapper(entity_t value,const Type& init_value){
            ((Type*)this->archtypes[value.archtype].components[type_id<Type>().index]) [value.index] = init_value;
        }

    public:
        Register(){};
        ~Register(){};

        bool valid(Entity e) const {
            const Entity index = get_index(e);
            if(
                index >= this->entity_value.size() ||
                this->entity_value[index].version != get_version(e)
            ) return false;
            return true;
        }
        
        // create entity
        Entity create(const compid_t comp_bitmap = 0) {
            const Entity result = createEntity();
            if(comp_bitmap != 0){
                const archtypeId_t archtype_index = this->getArchtypeIndex(comp_bitmap);
                this->entity_value[get_index(result)] = entity_t{ 
                    .index =   this->archtypes[archtype_index].allocate(result), 
                    .archtype = archtype_index, 
                    .version = get_version(result) 
                };
            }else
                this->entity_value[get_index(result)].index = null_entity_index;
            return result;
        }
        template<typename ... Args>
        Entity create(const Args& ... Argv) {
            const Entity result = create(componentsId<Args...>());
            const entity_t value = this->entity_value[get_index(result)];
            (init_wrapper(value, Argv) , ...);
            return result;
        }
        // destroy a entity entirly
        void destroy(Entity e) {
            if(!this->valid(e)) return;
            const Entity index = get_index(e);
            entity_t& value = this->entity_value[index];

            // entity is empty
            if(value.index != null_entity_index)
                this->destroyComponents(value);

            value.index = free_entity_index;
            value.archtype = free_archtype_index;
            free_entity_index = index;
        }

        void addComponents(Entity e,compid_t components_bitmap){
            if(!this->valid(e)) return;
            const Entity index = get_index(e);
            entity_t& value = this->entity_value[index];

            //compid_t components_bitmap = type_bit<T>();

            // empty entities contains invalid component id
            if(value.index != null_entity_index)
                components_bitmap |= this->archtypes_id[value.archtype];

            _changeComponent(e,components_bitmap);
        }

        void removeComponents(Entity e,compid_t components_bitmap){
            if(!this->valid(e)) return;
            const Entity index = get_index(e);
            entity_t& value = this->entity_value[index];

            if(value.index != null_entity_index){
                components_bitmap = this->archtypes_id[value.archtype] & ~components_bitmap;
                _changeComponent(e,components_bitmap);
            }

        }
        template<typename T>
        auto& getComponent(Entity e) const {
            if(!this->valid(e)) return;
            const Entity index = get_index(e);
            const entity_t value = this->entity_value[index];

            // empty entity
            assert(value.index != null_entity_index);
            // component does not exists
                  assert(this->archtypes[value.archtype].components[type_id<T>().index]);
            return ((T*)(this->archtypes[value.archtype].components[type_id<T>().index]))[value.index];
        }
        bool hasComponents(const Entity e,const compid_t components_bitmap) const {
            if(!this->valid(e)) return false;
            const Entity index = get_index(e);
            const entity_t value = this->entity_value[index];

            // no component
            if(value.index == null_entity_index)
                return false;
            return (this->archtypes_id[value.archtype] & components_bitmap) == components_bitmap;
        }
        // TODO: recives a chunk size, so gives call the callback multiple times 
        // with array of pointers to components plus entity id list and maximum size of arrays as arguments
        template<typename ... Types>
        void iterate(void(*func)(std::array<void*, sizeof...(Types) + 1>,size_t), const size_t chunk_size = 16) {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            const compid_t comps_index[sizeof...(Types)] = {(type_id<Types>().index)...};
            const size_t   comps_size[sizeof...(Types)] = {(type_id<Types>().size)...};
            std::array<void*, sizeof...(Types) + 1> args;
            for (archtypeId_t i = 0; i < this->archtypes_index; i++)
                // iterate through archtypes_id then check archtypes::size results in less cache miss
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    Archtype &arch = this->archtypes[i];
                    if(arch.size != 0)
                        for(size_t chunck_index=0; chunck_index < arch.size; chunck_index+=chunk_size) {
                            size_t comp_index = 0;
                            for (; comp_index < sizeof...(Types); comp_index++)
                                args[comp_index] = ((char*)(arch.components[comps_index[comp_index]])) + comps_size[comp_index] * chunck_index;
                            args[comp_index] = ((Entity*)(arch.components[32])) + chunck_index;
                            const size_t remaind = arch.size - chunck_index;
                            func(args,remaind<chunk_size?remaind:chunk_size);
                        }
                }
        }

        // TODO: recives a chunk size, so gives call the callback with multiple times 
        // with array of pointers to components and maximum size of arrays as argument
        template<typename ... Types>
        void iterate(void(*func)(Entity,Types...)) const {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            for (archtypeId_t i = 0; i < this->archtypes_index; i++)
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    // arch might be empty and unallocatted
                    Archtype &arch = this->archtypes[i];
                    for(size_t entity_index=0; entity_index < arch.size; entity_index++)
                        func( ((Entity*)arch.components[32])[entity_index], template_wrapper<Types>(entity_index) ...);
                }
        }
        /// @brief A helper function for job systems, searchs archtype after archtype
        /// @tparam ...Types components that must be included
        /// @param from begining entity value to start searching, pass 0 if want to begin from zero
        /// @param chunck_size maximum number of entities to pick in one single entity range
        /// @return range of available entities in value in same archtype exluding the end one, if begin and end was same
        // means it is an empty range and we found nothing.
        template<typename ... Types>
        entity_range findChunk(entity_t from, const size_t chunck_size) const {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            for (archtypeId_t i = from.archtype; i < this->archtypes_index; i++)
            {
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap)
                    if(from.index < this->archtypes[i].size)
                        return entity_range{
                            .begin=from.index, 
                            .end=std::min<archtypeId_t>(from.index+chunck_size, this->archtypes[i].size), 
                            .archtype=i
                        };
                from.index = 0;
            }
            return entity_range{.begin=from.index, .end=from.index, .archtype=null_archtype_index};
        }
        
        template<typename T>
        const T* getComponent2(const entity_t value) const {
            // empty entity
            assert(value.index != null_entity_index);
            void * const ptr = this->archtypes[value.archtype].components[type_id<T>().index];
            // component does not exists
            assert(ptr);
            return (const T*)ptr + value.index;
        }
        template<typename T>
        T* getComponent2(const entity_t value) {
            // empty entity
            assert(value.index != null_entity_index);
            void * const ptr = this->archtypes[value.archtype].components[type_id<T>().index];
            // component does not exists
            assert(ptr);
            return (T*)ptr + value.index;
        }
        Archtype& getArchtype2(const entity_t value){
             return this->archtypes[value.archtype];
        }
        const Archtype& getArchtype2(const entity_t value) const {
             return this->archtypes[value.archtype];
        }
        template<typename Type>
        void addSystem(Type* val){
            this->system.emplace_back(val);
        }
        void executeSystems(){
            for(auto& i:this->system)
                i->update();
        }
    };

} // namespace DOTS


#endif // REGISTER_HPP
