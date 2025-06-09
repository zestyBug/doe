#if !defined(EntityComponentManager_HPP)
#define EntityComponentManager_HPP

#include "defs.hpp"
#include <vector>
#include <array>
#include <memory>
#include "Archetype.hpp"
#include "cutil/bitset.hpp"
#include "cutil/span.hpp"
#include "cutil/unique_ptr.hpp"
#include "cutil/map.hpp"

namespace ECS
{
    // the class that holds all entities
    class EntityComponentManager final {
        friend class ThreadPool;
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t,allocator<entity_t>> entity_value{};

        std::vector<ArchetypeHolder,allocator<ArchetypeHolder>> archetypes{};
        map<span<TypeID>,Archetype> archetypeTypeMap{};


        int32_t FreeEntityIndex = Entity::null;
        uint32_t FreeArchetypeIndex = NullArchetypeIndex;


        /// @brief find or create a archetype with given types,
        /// @note type flag sensitive
        /// @param types list of types, throws invalid_argument exception on empty list
        Archetype* getOrCreateArchetype(span<TypeID> types);

        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const uint32_t archetypeIndex);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @note type flag sensitive
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithAddedComponents(Archetype *archetype,span<TypeID> componentTypeSet);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @note type flag sensitive
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithRemovedComponents(Archetype *archetype,span<TypeID> typeSetToRemove);

        /// @ref getArchetypeWithAddedComponents
        /// @note type flag sensitive
        Archetype* getArchetypeWithAddedComponent(Archetype* archetype,TypeID addedComponentType,uint32_t *indexInTypeArray = nullptr);

        /// @ref getArchetypeWithAddedComponents
        /// @note type flag sensitive
        Archetype* getArchetypeWithRemovedComponent(Archetype* archetype,TypeID addedComponentType,uint32_t *indexInOldTypeArray = nullptr);

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
            span<TypeID> types = componentTypesRaw<Types...>();
            iterate_helper(func,types);
        }
        void iterate(void(*func)(span<void*>,uint32_t)) {
            std::array<TypeID,1> types;
            types[0] = getTypeInfo<Entity>().value;
            iterate_helper(func,types);
        }

        bool valid(Entity e) const {
            if(
                !e.valid() || 
                (uint32_t)e.index() >= this->entity_value.size() ||
                this->entity_value.at(e.index()).version != e.version()
            ) return false;
            return true;
        }

        const entity_t& validate(Entity entity) const {
            if(!entity.valid())
                throw std::invalid_argument("validate(): invalid entity");
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
        /// @note type flag sensitive
        /// @param types types you are loking for
        Entity createEntity(span<TypeID> types);

        /// @brief releases components
        void removeComponents(Entity entity);

        /// @brief add list of components, components are initialized with default constructor
        /// @note not type flag sensitive
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void removeComponents(Entity entity, span<TypeID> componentTypeSet);

        /// @brief releases components
        void removeComponent(Entity entity, TypeID component);

        /// @brief add list of components, components are initialized with default constructor
        /// @note type flag sensitive
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void addComponents(Entity entity, span<TypeID> componentTypeSet);

        void addComponent(Entity entity, TypeID component);

        /// @brief simple linear check,
        /// @note type flag sensitive
        /// @param types type ordered
        bool hasComponents(Entity e,span<TypeID> types) const;

        /// @brief simple linear check
        /// @note type flag sensitive
        /// @param type type ordered
        bool hasComponent(Entity e,TypeID type) const;

        // optimized: using archetype bitmap
        template<typename T>
        bool hasComponent(Entity e) const {
            TypeID type = getTypeInfo<T>().value;
            return hasComponent(e,type);
        }

        /// @brief destroy a entity entirly, safe
        /// @param e entity, must contain a valid version
        void destroyEntity(Entity entity);

    protected:
        /**
         * @brief single threaded iteration helper
         * @note type flag sensitive
         * @param func a funtion pointer reciving the components pointers and count of entities in the componenets
         * @param types the components asked for iteration, can be empty list also can include 'Entity' as component
         */
        void iterate_helper(void(*func)(span<void*>,uint32_t),span<TypeID> types = span<TypeID>()) {
            size_t offset_buffer[types.size()];
            void* arg_buffer[types.size()];

            for (uint32_t i = 0; i < this->archetypes.size(); i++){
                Archetype *arch = this->archetypes[i].get();
                if(arch)
                    if(arch->hasComponents(types))
                    {
                        {
                            span<uint32_t> archOffsets{arch->offsets};
                            span<TypeID> archTypes{arch->types};
                            // fill offset_buffer
                            for(uint32_t typePosition=0;typePosition<types.size();typePosition++)
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
                            for(uint32_t chunckIndex = 0; chunckIndex < archChunks.size(); chunckIndex++) {
                                uint8_t * const chunkMemory = (uint8_t *)archChunks[chunckIndex].memory;
                                const uint32_t count = chunckIndex == lastChunkIndex ? lastChunkEntityCount : chunkCapacity ;
                                if(unlikely(chunkMemory == nullptr || count == 0))
                                    throw std::runtime_error("found an empty chunk");
                                for (size_t index = 0; index < types.size(); index++)
                                    arg_buffer[index] = chunkMemory + offset_buffer[index];
                                func( {arg_buffer,types.size()},count);
                            }
                        }
                    }
            }
        }

        // chain entity to free entities
        void recycleEntity(Entity entity);

        // recycle or create new
        Entity recycleEntity();
    };

} // namespace ECS


#endif
