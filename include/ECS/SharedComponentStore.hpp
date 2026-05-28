#if !defined(MANAGEDCOMPONENTSTORE_HPP)
#define MANAGEDCOMPONENTSTORE_HPP

#include "cutil/HashHelper.hpp"
#include "Base/TypeID.hpp"
#include "Base/Constants.hpp"
#include "Base/Chunk.hpp"
#include "Base/Version.hpp"
#include "Base/SharedComponent.hpp"

namespace ECS
{
    class SharedComponentStore
    {
    protected:
        static constexpr uint32_t AValidHashCode = 0x00000001;
        static constexpr uint32_t SkipCode = 0xFFFFFFFF;
        struct SharedComponentInfo {
            uint32_t referenceCount;
            Version version;
            uint32_t hash;
        };
        struct SharedComponentChunk {
            /// @brief MAGIC NUMBER
            static constexpr uint32_t InvalidIndex = 0xffffffff;
            /// @brief MAGIC NUMBER
            static constexpr uint32_t MaximumSize = UINT16_MAX;
            uint32_t capacity;
            uint32_t count;
            uint32_t freeSlotIndex;
            SharedComponentInfo *infos;
            uint32_t* hashes;
            uint32_t* hashes_index;
            alignas(64) uint8_t buffer[];
        };
        align_ptr<SharedComponentChunk> dataChunk[Constants::MaximumTypesCount];
        uint32_t sharedComponentVersion = 0;

        static TypeID getComponentType(SharedComponentIndex value) {
            return TypeManager::GetTypeInfoPointer()[value >> SharedComponentIndex::TypeIndexBitOffset].TypeIndex;
        }
        static uint32_t getTypeIndex(SharedComponentIndex value) {
            uint32_t typeIndex = value >> SharedComponentIndex::TypeIndexBitOffset;
            if(typeIndex > TypeManager::GetTypeCount())
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            return typeIndex;
        }
        static int getElementIndex(SharedComponentIndex value) {
            return value & 0xffff;
        }
        void initiate(uint32_t typeIndex);
        void resize(uint32_t typeIndex);
    public:
        SharedComponentStore() = default;
        ~SharedComponentStore();
        void *getPointer(SharedComponentIndex index) noexcept;
        SharedComponentIndex insert(TypeID type,void *data);
        SharedComponentIndex getDefaultValue(TypeID type);
        void addReference(SharedComponentIndex index, uint32_t num = 1);
        void removeReference(SharedComponentIndex index, uint32_t num = 1);
        void incrementVersion(SharedComponentIndex index);
        uint32_t getVersion(SharedComponentIndex index) const;
        void checkInternalConsistency();
    };
}

#endif
