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

class Test;

namespace ECS
{
    template <typename Type,Type>
    class ResourceGC;
    // the class that holds all entities
    class EntityComponentManager final {
        friend class ChunkJobFunction;
        template <typename Type,Type>
        friend class ResourceGC;
        friend class ::Test;
    protected:
        // array of entities value,
        // contains index of it archetype and it index in that archetype
        std::vector<entity_t,allocator<entity_t>> entity_value{};

        std::vector<ArchetypeHolder,allocator<ArchetypeHolder>> archetypes{};
        /// @brief hash does not include "Entity" component
        map<const_span<TypeID>,Archetype> archetypeTypeMap{};


        int32_t FreeEntityIndex = Entity::null;
        uint32_t FreeArchetypeIndex = NullArchetypeIndex;
        // global version buffer, used for any entity create/modify command
        version_t globalVersion = 1;


        /// @brief find or create a archetype with given types,
        /// @note type flag sensitive
        /// @attention types argument must include Entity component at index 0!
        /// @param types list of types, throws invalid_argument exception on empty list
        /// @return nullptr if types.size is less than 2
        Archetype* getOrCreateArchetype(const_span<TypeID> types);

        // destroys Archetype if empty otherwise nothing
        void destroyEmptyArchetype(const uint32_t archetypeIndex);

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @note type flag sensitive
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithAddedComponents(Archetype *archetype,const_span<TypeID> componentTypeSet) noexcept;

        /// @brief add component to an entity or in other word, move entity to another archetype, exception handled
        /// @note not type flag sensitive
        /// @param srcArchetype contains src types
        /// @param componentTypeSet dont feed empty list, there is no quick size check for branch optimization!
        /// @return return another archtype or itself if nochange detected
        Archetype* getArchetypeWithRemovedComponents(Archetype *archetype,const_span<TypeID> typeSetToRemove) noexcept;

        /// @ref getArchetypeWithAddedComponents
        /// @note type flag sensitive
        Archetype* getArchetypeWithAddedComponent(Archetype* archetype,TypeID addedComponentType,uint32_t *indexInTypeArray = nullptr) noexcept;

        /// @ref getArchetypeWithAddedComponents
        /// @note not type flag sensitive
        /// @attention an exception will be thrown by getOrCreateArchetype if Entity is passed as argument for removedComponentType
        Archetype* getArchetypeWithRemovedComponent(Archetype* archetype,TypeID removedComponentType,uint32_t *indexInOldTypeArray = nullptr) noexcept;

        /// @brief add Entity component to the array and calls getOrCreateArchetype
        /// @attention input validity is not checked! 
        void Helper_allocateInArchetype(const_span<TypeID> componentTypeSet,entity_t *srcValue, Entity entity) noexcept;
        /// @brief simple wrapper for boiler plate code
        void Helper_allocateInArchetype(Archetype *newArchetype,entity_t *srcValue, Entity entity) noexcept;
        /// @brief simple wrapper for boiler plate code
        /// @attention input validity is not checked!
        void Helper_removeFromArchetype(Archetype *srcArchetype,entity_t *srcValue) noexcept;
        /// @brief manages move to new archetype
        /// @attention input validity is not checked!
        /// @return index in new archetype
        void Helper_moveEntityToNewArchetype(Archetype *__restrict__ newArchetype,Archetype *__restrict__srcArchetype,entity_t *srcValue) noexcept;


        void updateVersion() { ECS::updateVersion(this->globalVersion); }
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

        version_t getVersion() const { return this->globalVersion; }

        template<typename ... Types>
        void iterate(void(*func)(const_span<void*>,uint32_t)) {
            const_span<TypeID> types = componentTypesRaw<Types...>();
            iterate_helper(func,types);
        }
        void iterate(void(*func)(const_span<void*>,uint32_t)) {
            std::array<TypeID,1> types;
            types[0] = 0;// getTypeID<Entity>()
            iterate_helper(func,types);
        }

        /// @brief Helper function for debugging
        /// @return may returns null if has no archetype
        const Archetype* getArchetype(Entity entity) const;

    protected:
        /// @brief Helper function for debugging
        /// @return may returns null if has no archetype
        Archetype* getArchetype(Entity entity);

    public:
        /// @brief checking entity validation before sending arguments avoid exceptions
        /// @return true if entity index exists and version is currect
        bool valid(Entity entity) const noexcept {
            if(
                !entity.valid() || 
                (uint32_t)entity.index() >= this->entity_value.size() ||
                this->entity_value.at(entity.index()).version != entity.version()
            ) return false;
            return true;
        }

        /// @brief creates an empty entity (allocate on entity_value array)
        /// @return archetype is set to null (safe)
        Entity createEntity();

        /// @brief create entity and stores entity value and initilize entity
        /// @note type flag sensitive
        /// @param types types you are loking for
        Entity createEntity(const_span<TypeID> types);

        /// @brief releases components
        void removeComponents(Entity entity);

        /// @brief add list of components, components are initialized with default constructor
        /// @note not type flag sensitive
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void removeComponents(Entity entity, const_span<TypeID> componentTypeSet);

        /// @brief releases components
        void removeComponent(Entity entity, TypeID component);

        /// @brief add list of components, components are initialized with default constructor
        /// @note type flag sensitive
        /// @param entity entity, can belong to no archetype
        /// @param componentTypeSet set of components to be added
        void addComponents(Entity entity, const_span<TypeID> componentTypeSet);

        void addComponent(Entity entity, TypeID component);

        /// @brief simple linear check,
        /// @note type flag sensitive
        /// @param types type ordered
        /// @return true if has types[0] AND types[1] AND ...
        bool hasComponents(Entity entity,const_span<TypeID> types) const;

        /// @brief simple linear check
        /// @note type flag sensitive
        /// @param type type ordered
        bool hasComponent(Entity entity,TypeID type) const;

        // optimized: using archetype bitmap
        template<typename T>
        bool hasComponent(Entity entity) const {
            TypeID type = getTypeInfo<T>().value;
            return hasComponent(entity,type);
        }

        /// @brief get R/W access to component
        /// @param entity valid entity
        /// @param type requested component type
        /// @return nullptr if entity doesnt contain this component
        void* getComponent(Entity entity,TypeID type);
        /// @brief get RO access to single component
        /// @param entity valid entity
        /// @param type requested component type
        /// @return nullptr if entity doesnt contain this component
        const void* getComponent(Entity entity,TypeID type) const;
        
        template<typename T>
        T& getComponent(Entity entity){
            void *ptr;
            TypeID type = getTypeInfo<T>().value;
            ptr = getComponent(entity,type);
            if(!ptr)
                throw std::runtime_error("getComponent(): component not found");
            return *(T*)ptr;
        }

        bool hasArchetype(Entity entity) const;

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
        void iterate_helper(void(*func)(const_span<void*>,uint32_t),const_span<TypeID> types = const_span<TypeID>()) {
            size_t offset_buffer[types.size()];
            void* arg_buffer[types.size()];

            for (uint32_t i = 0; i < this->archetypes.size(); i++){
                Archetype *arch = this->archetypes[i].get();
                if(arch)
                    if(arch->hasComponents(types))
                    {
                        {
                            const_span<uint32_t> archOffsets{arch->offsets};
                            const_span<TypeID> archTypes{arch->types};
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
                            const_span<Chunk> archChunks{arch->chunksData};
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

        /// @brief chain entity to free entities
        void recycleEntity(Entity entity);

        /// @brief recycle or create new
        Entity recycleEntity();
    };

} // namespace ECS


#endif
