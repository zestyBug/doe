#if !defined(ARCHTYPECHUNKDATA_HPP)
#define ARCHTYPECHUNKDATA_HPP

#include "cutil/span.hpp"
#include "cutil/basics.hpp"
#include "Base/Chunk.hpp"
#include "Base/SharedComponent.hpp"
#include "Base/Version.hpp"
namespace ECS
{
    class Archetype;
    /** @brief there is a component version for each compopnent. 
     * any write access, whitout check for real value change, causes to update version to lastest version.
     * for performance reason, this value is not per entity/component but per chunk/component.
     * @note using version check may brings efficiency to your system by avoiding unnecesary computation.
     */
    struct ArchetypeChunkData final
    {
    private:
        friend class Archetype;
        // SOA for chunks in a archetype : index, shared component index, version, count
        align_ptr<uint8_t[]> buck;
        Chunk** _Chunk = nullptr;
        Version* _ChangeVersion = nullptr;
        SharedComponentIndex* _SharedComponentValue = nullptr;

        // maximum number of chunks information that can be stored, before grow
        uint32_t _capacity=0;
        
        // number of chunks allocated
        uint32_t _count=0;
        const uint32_t sharedComponentCount;
        // total number (tags and shared components included)
        const uint32_t componentCount;

        // ChangeVersions and SharedComponentValues stored like:
        //  Type[        0         ]: [chunk[0] ... chunk[capacity - 1]]
        //  Type[       ...        ]: [         ...                    ]
        //  Type[componentCount - 1]: [chunk[0] ... chunk[capacity - 1]]

    public:
        ArchetypeChunkData(uint32_t _component_count, uint32_t _shared_component_count): sharedComponentCount{_shared_component_count}, componentCount{_component_count} {
            if(_component_count < 1)
                throw std::invalid_argument("ArchetypeChunkData(): zero component archetype chunk data");
        }
        ~ArchetypeChunkData() = default;
        ArchetypeChunkData(const ArchetypeChunkData&)= delete;
        ArchetypeChunkData& operator    = (const ArchetypeChunkData&)= delete;
        ArchetypeChunkData(ArchetypeChunkData&&)= default;

        inline bool empty() const {
            return this->_count < 1;
        }
        inline uint32_t capacity() const {
            return this->_capacity;
        }
        inline uint32_t count() const {
            return this->_count;
        }
        Chunk* operator[](uint32_t i) {
            if(i >= this->_count)
                throw std::out_of_range("ArchetypeChunkData::operator[]");
            return _Chunk[i];
        }
        inline const_span<Chunk*> getChunkIndexArray() { return {_Chunk, this->_count}; }
        span<Version> getChangeVersionArrayForType(uint32_t component_index_in_archtype);
        Version getChangeVersion(uint32_t component_index_in_archtype, uint32_t index);
        // set version of all components in a chunk
        void setAllChangeVersion(uint32_t index, Version version);
        void setChangeVersion(uint32_t component_index_in_archtype, uint32_t index, Version version);
        const_span<SharedComponentIndex> getSharedComponentValueArrayForType(uint32_t shared_component_index_in_archtype);
        void setSharedComponentValue(uint32_t shared_component_index_in_archtype, uint32_t index, SharedComponentIndex value);
        SharedComponentIndex getSharedComponentValue(uint32_t shared_component_index_in_archtype, uint32_t index);
        SharedComponentValues getSharedComponentValues(uint32_t index);
    private:
        void popBack();
        void removeAtSwapBack(uint32_t index);
        /// @brief add new chunk at end of list
        /// @param version the version to be set
        void add(Chunk* chunk, SharedComponentValues sharedComponentIndices = {}, Version version = 0);
    protected:
        // dont call it on your own!
        void grow(uint32_t new_capacity);
    };

} // namespace ECS




#endif // ARCHTYPECHUNKDATA_HPP
