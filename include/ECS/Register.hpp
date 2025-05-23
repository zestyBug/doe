#if !defined(REGISTER_HPP)
#define REGISTER_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <assert.h>
#include <memory>
#include "Archetype.hpp"
#include "cutil/bitset.hpp"
#include "cutil/SmallVector.hpp"
#include "cutil/span.hpp"
#include "cutil/unique_ptr.hpp"
#include "ListMap.hpp"

namespace DOTS
{
    // the class that holds all entities
    class Register final {
        friend class EntityComponentSystem;
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t> entity_value{};

        std::vector<unique_ptr<Archetype>> archetypes{};
        ArchetypeListMap archetypeTypeMap{};


        uint32_t freeEntityIndex = Entity::maxEntityCount;
        uint32_t freeArchetypeIndex = nullArchetypeIndex;


        /// @brief find or create a archetype with given types,
        /// @param types list of types, throws invalid_argument exception on empty list
        Archetype* getOrCreateArchetype(span<TypeIndex> types);

        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const uint32_t archetypeIndex);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithAddedComponents(Archetype *archetype,span<TypeIndex> componentTypeSet);
        /// @ref getArchetypeWithAddedComponents
        Archetype* getArchetypeWithAddedComponent (Archetype* archetype,TypeIndex addedComponentType);

    /*
     * Only Public function verfy inputs validity.
     */

    public:
        Register(){
            archetypes.reserve(4);
            archetypeTypeMap.init(4);
            entity_value.reserve(128);
        };
        ~Register(){
            for(const auto& arch_ptr: archetypes){
                Archetype *arch = arch_ptr.get();
                if(arch){
                    if(!arch)
                        continue;
                    span<uint16_t> sizes = arch->sizeOfs;
                    span<Chunk> chunks = arch->chunksData;
                    span<uint32_t> offsets = arch->offsets;
                    span<TypeIndex> types = arch->types;
                    for (size_t typeIndex = 0; typeIndex < arch->nonZeroSizedTypesCount(); typeIndex++)
                    {
                        auto destructor =  getTypeInfo(types[typeIndex]).destructor;
                        for (size_t chunkIndex = 0; chunkIndex < chunks.size(); chunkIndex++)
                        {
                            uint8_t * const componentMemory = 
                                (uint8_t*)(chunks[chunkIndex].memory) + offsets[typeIndex];
                            const size_t entityCount = (chunkIndex + 1) == chunks.size() ? 
                                arch->lastChunkEntityCount : arch->chunkCapacity;
                            const size_t typeSize = sizes[typeIndex];
                            for (size_t valueIndex = 0; valueIndex < entityCount; valueIndex++)
                                destructor(componentMemory + typeSize*valueIndex);
                            
                        }
                        
                    }
                    
                }
            }
        }

        bool valid(Entity e) const {
            if(
                e.index() >= this->entity_value.size() ||
                this->entity_value.at(e.index()).version != e.version()
            ) return false;
            return true;
        }

        const entity_t& validate(Entity entity) const {
            const entity_t& value = this->entity_value.at(entity.index());
            // version check to avoid double destroy
            if(value.version != entity.version())
                throw std::invalid_argument("validate(): entity doesnt exist anymore");
            return value;
        }
        entity_t& validate(Entity entity) {
            entity_t& value = this->entity_value.at(entity.index());
            // version check to avoid double destroy
            if(value.version != entity.version())
                throw std::invalid_argument("validate(): entity doesnt exist anymore");
            return value;
        }

        /// @brief creates an empty entity (allocate on entity_value array)
        /// @return archetype is set to null (safe)
        Entity createEntity();

        template<typename ... Args>
        Entity createEntity() {
            std::vector<TypeIndex> types = componentTypes<Args...>();
            const Entity result = this->createEntity(types);
            return result;
        }

        /// @brief releases components
        void removeComponents(Entity entity);

        /// @brief destroy a entity entirly, safe
        /// @param e entity, must contain a valid version
        void destroyEntity(Entity entity);

        /// @brief add list of components, components are initialized with default constructor
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void addComponent(Entity entity, span<TypeIndex> componentTypeSet);

        /// @brief simple linear check
        /// @param types type flags are ignored!
        bool hasComponents(Entity e,span<TypeIndex> types) const;

        /// @brief simple linear check
        /// @param type type flag is ignored!
        bool hasComponent(Entity e,TypeIndex type) const;

        // optimized: using archetype bitmap
        template<typename T>
        bool hasComponent(Entity e) const {
            TypeIndex type = getTypeInfo<T>().value;
            return hasComponent(e,type);
        }

    protected:

        /// @brief create entity and stores entity value and initilize entity
        /// @param types types you are loking for
        Entity createEntity(span<TypeIndex> types);

        // chain entity to free entities
        void recycleEntity(Entity entity);

        // recycle or create new
        Entity recycleEntity();

        /// @brief in-archetype move operation, handles deconstruction + memcpy by itself
        /// @param entity value of srcIndex to be updated
        static Entity moveComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex);

        /// @brief in-archetype move operation, handles only memcpy not destruction
        /// @param entity value of srcIndex to be updated
        static Entity replaceComponentValues(Archetype *archetype, uint32_t dstIndex, uint32_t srcIndex);

        /// @brief between-archetype move operation, handles deconstruction + construction + memcpy by itself
        /// @param entity value of srcIndex to be updated
        static Entity moveComponentValues(Archetype *dstArchetype, uint32_t dstIndex,Archetype *srcArchetype, uint32_t srcIndex);

        /// @brief call destructor function of components
        /// @param index index inside archetype
        static void callComponentDestructor(Archetype *arch, uint32_t entityIndex);

        /// @brief call constructor function of components
        /// @param index index inside archetype
        /// @param entity entity value to be storeds
        static void callComponentConstructor(Archetype *arch, uint32_t entityIndex, Entity e = Entity::null);
    };

} // namespace DOTS


#endif
