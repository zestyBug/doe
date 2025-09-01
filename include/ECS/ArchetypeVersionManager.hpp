#if !defined(ARCHTYPECHUNKDATA_HPP)
#define ARCHTYPECHUNKDATA_HPP

#include "cutil/span.hpp"
#include "cutil/basics.hpp"
#include "defs.hpp"

namespace ECS
{

    /** @brief there is a component version for each compopnent. 
     * any write access, whitout check for real value change, causes to update version to lastest version.
     * for performance reason, this value is not per entity/component but per chunk/component.
     * @note using version check may brings efficiency to your system by avoiding unnecesary computation.
     */
    class ArchetypeVersionManager final
    {
        // SOA for chunks in a archetype : index, shared component index, version, count
        align_ptr<uint8_t[]> buck;

        // maximum number of chunks information that can be stored, before grow
        uint32_t _capacity=0;
        
        // number of chunks allocated
        uint32_t _count=0;
        const uint32_t componentCount=0;

        // version value: (suitable for single type iteration)
        // note all type are same in archetype structure
        //    [ comp[0]:                  [chunk[0] ... chunk[_capacity - 1]]
        //      comp[...]:                ...
        //      comp[componentCount - 1]: [chunk[0] ... chunk[_capacity - 1]]

        friend class Archetype;

    public:
        ArchetypeVersionManager(uint32_t _component_count): componentCount{_component_count} {
            if(_component_count < 1)
                throw std::invalid_argument("ArchetypeVersionManager(): zero component archetype chunk data");
        }
        ~ArchetypeVersionManager() = default;

        ArchetypeVersionManager(const ArchetypeVersionManager&)= delete;
        ArchetypeVersionManager& operator = (const ArchetypeVersionManager&)= delete;
        ArchetypeVersionManager(ArchetypeVersionManager&&)= default;
    protected:

        version_t& _ChangeVersion(uint32_t index=0){
            return ((version_t*)(this->buck.get()))[index];
        }
    public:
        inline bool empty() const {
            return this->_count < 1;
        }
        inline uint32_t capacity() const {
            return this->_capacity;
        }
        inline uint32_t count() const {
            return this->_count;
        }

        span<version_t> getChangeVersionArrayForType(uint32_t component_index);
        version_t& getChangeVersion(uint32_t component_index, uint32_t chunkIndex);
        // set version of all components in a chunk
        void setAllChangeVersion(uint32_t chunkIndex, version_t version);

        void popBack();

        /// @brief add new chunk at end of list
        /// @param version the version to be set
        void add(version_t version = 0);
    protected:
        // dont call it on your own!
        void grow(uint32_t new_capacity);
    };

} // namespace ECS




#endif // ARCHTYPECHUNKDATA_HPP
