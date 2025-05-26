#if !defined(EntityComponentManager_HPP)
#define EntityComponentManager_HPP

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
#include "HashMap.hpp"

namespace ECS
{
    // the class that holds all entities
    class EntityComponentManager final {
        friend class EntityComponentSystem;
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t> entity_value{};

        std::vector<unique_ptr<Archetype>> archetypes{};
        ArchetypeHashMap archetypeTypeMap{};


        uint32_t freeEntityIndex = Entity::maxEntityCount;
        uint32_t freeArchetypeIndex = nullArchetypeIndex;


        /// @brief find or create a archetype with given types,
        /// @param types list of types, throws invalid_argument exception on empty list
        Archetype* getOrCreateArchetype(span<Type> types);

        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const uint32_t archetypeIndex);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithAddedComponents(Archetype *archetype,span<Type> componentTypeSet);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithRemovedComponents(Archetype *archetype,span<Type> typeSetToRemove);

        /// @ref getArchetypeWithAddedComponents
        Archetype* getArchetypeWithAddedComponent(Archetype* archetype,Type addedComponentType,uint32_t *indexInTypeArray = nullptr);

        /// @ref getArchetypeWithAddedComponents
        Archetype* getArchetypeWithRemovedComponent(Archetype* archetype,Type addedComponentType,uint32_t *indexInOldTypeArray = nullptr);

    /*
     * Only Public function verfy inputs validity.
     */

    public:
        EntityComponentManager(){
            archetypes.reserve(4);
            archetypeTypeMap.init(4);
            entity_value.reserve(128);
        };
        ~EntityComponentManager(){
        }

        template<typename ... Types>
        void iterate(void(*func)(span<void*>,uint32_t)) {
            span<Type> types = componentTypes<Types...>();
            std::array<size_t, (sizeof...(Types))> offset_buffer;
            std::array<void*, (sizeof...(Types))> arg_buffer;

            for (uint32_t i = 0; i < this->archetypes.size(); i++){
                Archetype *arch = this->archetypes[i].get();
                if(arch)
                    if(arch->hasComponents(types))
                    {
                        {
                            span<uint32_t> archOffsets{arch->offsets};
                            span<Type> archTypes{arch->types};
                            // fill offset_buffer
                            for(uint32_t typePosition=0;typePosition<offset_buffer.size();typePosition++)
                            {
                                for(uint32_t archetypeTypeIndex=0;archetypeTypeIndex<archTypes.size();archetypeTypeIndex++)
                                    if(types[typePosition].exactSame(archTypes[archetypeTypeIndex])){
                                        offset_buffer[typePosition] = archOffsets[archetypeTypeIndex];
                                        goto JMPING;
                                    }
                                throw std::runtime_error("failed to find a component offset");
                                JMPING:;
                            }
                        }
                        {
                            span<Chunk> archChunks{arch->chunksData};
                            const uint32_t lastChunkEntityCount = arch->lastChunkEntityCount;
                            const uint32_t chunkCapacity = arch->chunkCapacity;
                            const uint32_t lastChunkIndex = archChunks.size() - 1;
                            for(size_t chunckIndex = 0; chunckIndex < archChunks.size(); chunckIndex++) {
                                uint8_t * const chunkMemory = (uint8_t *)archChunks[chunckIndex].memory;
                                const uint32_t count = chunckIndex == lastChunkIndex ? lastChunkEntityCount : chunkCapacity ;
                                if(unlikely(chunkMemory == nullptr || count == 0))
                                    throw std::runtime_error("found an empty chunk");
                                for (size_t index = 0; index < arg_buffer.size(); index++)
                                    arg_buffer[index] = chunkMemory + offset_buffer[index];
                                func( arg_buffer,count);
                            }
                        }
                    }
            }
        }
        void iterate(void(*func)(span<void*>,uint32_t)) {
            void* arg_buffer;
            for (uint32_t i = 0; i < this->archetypes.size(); i++){
                Archetype *arch = this->archetypes[i].get();
                if(arch)
                {
                            span<Chunk> archChunks{arch->chunksData};
                            uint32_t entity_offset = arch->offsets.at(0);
                            const uint32_t lastChunkEntityCount = arch->lastChunkEntityCount;
                            const uint32_t chunkCapacity = arch->chunkCapacity;
                            const uint32_t lastChunkIndex = archChunks.size() - 1;
                            for(size_t chunckIndex = 0; chunckIndex < archChunks.size(); chunckIndex++) {
                                uint8_t * const chunkMemory = (uint8_t *)archChunks[chunckIndex].memory;
                                const uint32_t count = chunckIndex == lastChunkIndex ? lastChunkEntityCount : chunkCapacity ;
                                if(unlikely(chunkMemory == nullptr || count == 0))
                                    throw std::runtime_error("found an empty chunk");
                                arg_buffer = chunkMemory + entity_offset;
                                func({&arg_buffer,1},count);
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

        /// @brief create entity and stores entity value and initilize entity
        /// @param types types you are loking for
        Entity createEntity(span<Type> types);

        /// @brief releases components
        void removeComponents(Entity entity);

        /// @brief destroy a entity entirly, safe
        /// @param e entity, must contain a valid version
        void destroyEntity(Entity entity);

        /// @brief add list of components, components are initialized with default constructor
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void addComponents(Entity entity, span<Type> componentTypeSet);

        /// @brief add list of components, components are initialized with default constructor
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void removeComponents(Entity entity, span<Type> componentTypeSet);

        /// @brief simple linear check,
        /// @param types type flags are NOT ignored!
        bool hasComponents(Entity e,span<Type> types) const;

        /// @brief simple linear check
        /// @param type type flag is NOT ignored!
        bool hasComponent(Entity e,Type type) const;

        // optimized: using archetype bitmap
        template<typename T>
        bool hasComponent(Entity e) const {
            Type type = getTypeInfo<T>().value;
            return hasComponent(e,type);
        }

    protected:

        // chain entity to free entities
        void recycleEntity(Entity entity);

        // recycle or create new
        Entity recycleEntity();
    };

} // namespace ECS


#endif
