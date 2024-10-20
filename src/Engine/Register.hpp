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
        static constexpr entity_t null_entity_id = 0xffffff;
        static constexpr archtypeId_t null_archtype_index = 0xffffffff;
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

        entity_t free_entity_index = null_entity_id;
        archtypeId_t free_archtype_index = null_archtype_index;

        // find or create a archtype with given types, 
        // NOTE: without initializing it
        archtypeId_t getArchtypeIndex(compid_t comp_bitmap){
            archtypeId_t archtype_index;

            for(size_t i = 0;i < this->archtypes_index;i++){
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
            if(free_entity_index == null_entity_id){
                size_t index = entity_value.size();
                assert(index < 0xffffff);
                entity_value.push_back(null_entity_id);
                result = index;
            }else{
                const entity_t prev_val = entity_value[free_entity_index];
                unsigned int e_version = get_version(prev_val);
                result = free_entity_index | (e_version + 0x1000000);
                free_entity_index = get_index(prev_val);
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
            archtypeId_t archtype_index = get_archtype_index(value);
            entity_t index = get_index(value);
            const entity_t modified = get_index( this->archtypes[archtype_index].destroy2(index) );
            // if it index was filled with other entity
            if(modified != null_entity_id)
                this->entity_value[modified] = index | (archtype_index<<24);
            // otherwise either archtype was empty or entity was indexed last one in array
            destroyEmptyComponent(archtype_index);
        }
        // removes comonents not entity itsel, recives entity value (index + archtype)
        void destroyComponents(entity_t value){
            archtypeId_t archtype_index = get_archtype_index(value);
            entity_t index = get_index(value);
            const entity_t modified = get_index( this->archtypes[archtype_index].destroy(index) );
            // if it index was filled with other entity
            if(modified != null_entity_id)
                this->entity_value[modified] = index | (archtype_index<<24);
            // otherwise either archtype was empty or entity was indexed last one in array
            destroyEmptyComponent(archtype_index);
        }
        // charges archtype, recives entity id and components bitmap
        void _changeComponent(Entity entity, compid_t new_component_bitmap){
            compid_t old_component_bitmap;
            const entity_t entity_index = get_index(entity);
            assert(entity_index < this->entity_value.size());
            entity_t old_value = this->entity_value[entity_index];
            const entity_t old_value_index = get_index(old_value);
            // WARN: empty entities contains invalid archtype id
            const archtypeId_t old_archtype_index = get_archtype_index(old_value);

            if(old_value_index == null_entity_id){
                old_component_bitmap = 0;
            }else{
                old_component_bitmap = this->archtypes_id[old_archtype_index];
            }
            
            if(old_component_bitmap == new_component_bitmap){// nothing to be done
            }else if(new_component_bitmap == 0){// change to no component
                if(old_component_bitmap != 0){
                    // remove all components
                    destroyComponents(old_value);
                }
                this->entity_value[entity_index] = null_entity_id;
            }else{
                const archtypeId_t new_archtype_index = getArchtypeIndex(new_component_bitmap);
                const entity_t new_value_index = this->archtypes[new_archtype_index].allocate(entity);
                this->entity_value[entity_index] = new_value_index | (new_archtype_index << 24);
                // literally creates a new entity
                if(old_component_bitmap == 0){
                }else{ // moving old components
                    for (compid_t i = 0; i < 32; i++){
                        if(old_component_bitmap & 1) {
                            const comp_info component_info = type_id(i);
                            // TODO: does *_component_bitmap means we can access component array blindly? i mean without null pointer check.
                            void *src = ((char*)(this->archtypes[old_archtype_index].components[i])) + (old_value_index * component_info.size);
                            if(new_component_bitmap & 1){
                                void *dst = ((char*)(this->archtypes[new_archtype_index].components[i])) + (new_value_index * component_info.size);
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
        Type template_wrapper(std::array<void*,33> &comps,size_t entity_index){
            return ((std::add_pointer_t<std::remove_const_t<std::remove_reference_t<Type>>>)comps[type_id<Type>().index]) [entity_index];
        }

    public:
        Register(){};
        ~Register(){};
        
        // create empty entity
        Entity create(){
            Entity result = createEntity();
            entity_value[get_index(result)] = null_entity_id;
            return result;
        }
        // create entity with given Components
        template<typename ... Args>
        Entity create() {
            compid_t comp_bitmap = (type_bit<Args>() | ...);

            const archtypeId_t archtype_index = getArchtypeIndex(comp_bitmap);

            const Entity result = createEntity();
            const entity_t value = this->archtypes[archtype_index].allocate(result) | (archtype_index << 24);
            this->entity_value[get_index(result)] = value;
            return result;
        }
        // destroy a entity entirly
        void destroy(Entity e) {
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t old_value = this->entity_value[index];

            this->entity_value[index] = free_entity_index | get_version(e);
            free_entity_index = index;

            // entity is empty
            if(get_index(old_value) == null_entity_id)
                return;

            destroyComponents(old_value);
        }

        template<typename T>
        void addComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            entity_t old_value = this->entity_value[index];
            compid_t components_bitmap = type_bit<T>();

            // empty entities may contain invalid component id
            if(get_index(old_value) != null_entity_id){
                const archtypeId_t old_archtype_index = get_archtype_index(old_value);
                components_bitmap |= this->archtypes_id[old_archtype_index];
            }

            _changeComponent(e,components_bitmap);
        }
        template<typename T>
        void removeComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t old_value = this->entity_value[index];
            compid_t new_component_bitmap=0;

            if(get_index(old_value) != null_entity_id){
                const archtypeId_t archtype_index = get_archtype_index(old_value);
                new_component_bitmap = this->archtypes_id[archtype_index] & ~type_bit<T>();
            }

            _changeComponent(e,new_component_bitmap);
        }
        template<typename T>
        auto& getComponent(Entity e) const {
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t value = this->entity_value[index];
            const archtypeId_t archtype_index = get_archtype_index(value);

            // empty entity
            assert(get_index(value) != null_entity_id);
            // component does not exists
            assert(this->archtypes[archtype_index].components[type_id<T>().index]);
            return ((T*)(this->archtypes[archtype_index].components[type_id<T>().index]))[get_index(value)];
        }
        template<typename T>
        bool hasComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t value = this->entity_value[index];

            // no component
            if(get_index(value) == null_entity_id)
                return false;
            return this->archtypes_id[get_archtype_index(value)] & type_bit<T>();
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
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    Archtype &arch = this->archtypes[i];
                    if(arch.size != 0)
                        for(size_t chunck_index=0; chunck_index < arch.size; chunck_index+=chunk_size) {
                            size_t comp_index = 0;
                            for (; comp_index < sizeof...(Types); comp_index++)
                                args[comp_index] = ((char*)(arch.components[comps_index[comp_index]])) + comps_size[comp_index] * chunck_index;
                            args[comp_index] = ((entity_t*)(arch.components[32])) + chunck_index;
                            const size_t remaind = arch.size - chunck_index;
                            func(args,remaind<chunk_size?remaind:chunk_size);
                        }
                }
        }

        // TODO: recives a chunk size, so gives call the callback with multiple times 
        // with array of pointers to components and maximum size of arrays as argument
        template<typename ... Types>
        void iterate(void(*func)(Entity,Types...)) {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            for (archtypeId_t i = 0; i < this->archtypes_index; i++)
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    // arch might be empty and unallocatted
                    Archtype &arch = this->archtypes[i];
                    for(size_t entity_index=0; entity_index < arch.size; entity_index++)
                        func( ((Entity*)arch.components[32])[entity_index], template_wrapper<Types>(arch.components,entity_index) ...);
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
            entity_t entity_index = get_index(from);
            const entity_t archtype_index = get_archtype_index(from);
            for (archtypeId_t i = archtype_index; i < this->archtypes_index; i++){
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    if(entity_index < this->archtypes[i].size){
                        return {(i<<24) | entity_index,(i<<24) | (entity_t)std::min(this->archtypes[i].size, entity_index+chunck_size)};
                    }
                }
                entity_index = 0;
            }
            return {from,from};
        }
        
        template<typename T>
        auto* getComponent2(entity_t value) const {
            const archtypeId_t archtype_index = get_archtype_index(value);
            const entity_t entity_index = get_index(value);

            // empty entity
            assert(entity_index != null_entity_id);
            void * const ptr = this->archtypes[archtype_index].components[type_id<T>().index];
            // component does not exists
            assert(ptr);
            return (T*)ptr + entity_index;
        }
        template<typename Type>
        void addSystem(){
            this->system.emplace_back(new Type());
        }
        void executeSystems(){
            for(auto& i:this->system)
                i->update();
        }
    };

} // namespace DOTS


#endif // REGISTER_HPP
