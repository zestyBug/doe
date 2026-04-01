#if !defined(MANAGEDCOMPONENTSTORE_HPP)
#define MANAGEDCOMPONENTSTORE_HPP

#include "cutil/range.hpp"
#include "Base/TypeID.hpp"
#include "Base/Chunk.hpp"
#include "Base/Version.hpp"
#include "Base/SharedComponent.hpp"

namespace ECS
{
    class SharedComponentStore
    {
    protected:
        struct SharedComponentInfo {
            uint32_t refCount;
            Version version;
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
            alignas(64) uint8_t buffer[];
        };
        std::array<align_ptr<SharedComponentChunk>,TypeID::MaximumTypesCount> dataChunk;
        TypeID getComponentType(SharedComponentIndex value) {
            return TypeManager::GetTypeInfoPointer()[value >> SharedComponentIndex::TypeIndexBitOffset].TypeIndex;
        }
        uint32_t getTypeIndex(SharedComponentIndex value) {
            return value >> SharedComponentIndex::TypeIndexBitOffset;
        }
        static int getElementIndex(SharedComponentIndex value) {
            return value & 0xffff;
        }
        static uint32_t calculateCapacity(uint32_t chunkSize, uint32_t typeSize){
            uint32_t buffer = chunkSize / (typeSize + sizeof(SharedComponentInfo));
            if(chunkSize <= 64)
                return 0;
            while (chunkSize < (alignTo64(typeSize*buffer) + sizeof(SharedComponentInfo)*buffer))
                buffer --;
            return buffer;
        }
        void resizeIfNeeded(uint32_t typeIndex){
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            uint32_t freeIndex;
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr){
                // buffer size
                const uint32_t buffer2 = std::max<uint32_t>(1024,64*typeSize);
                // count
                const uint32_t buffer1 = buffer2 / typeSize;
                ptr = (SharedComponentChunk*) allocator().allocate(buffer2 + buffer1 * sizeof(SharedComponentInfo));
                ptr->capacity = buffer1;
                ptr->count = 0;
                ptr->freeSlotIndex = SharedComponentChunk::InvalidIndex;
                ptr->infos = (SharedComponentInfo*)(((uint8_t*)ptr)+buffer2);
                memset(ptr->infos, 0, buffer1*sizeof(SharedComponentInfo));
                dataChunk[typeIndex].reset(ptr);
            }else{
                if(ptr->capacity == ptr->count && ptr->freeSlotIndex == SharedComponentChunk::InvalidIndex)
                {
                    // count
                    const uint32_t buffer1 = ptr->capacity * 2;
                #if VERBOSE
                    printf("SharedComponentStore::resize(): %u => %u\n", ptr->capacity, buffer1);
                #endif
                    // buffer size
                    const uint32_t buffer2 = buffer1 * typeSize;
                    SharedComponentChunk* ptr2 = (SharedComponentChunk*) allocator().allocate(buffer2 + buffer1 * sizeof(SharedComponentInfo));
                    ptr2->capacity = buffer1;
                    ptr2->count = ptr->count;
                    ptr2->freeSlotIndex = SharedComponentChunk::InvalidIndex;
                    ptr2->infos = (SharedComponentInfo*)((uint8_t*)ptr2+buffer2);
                    memcpy(ptr2->buffer,ptr->buffer,typeSize*ptr->capacity);
                    memcpy(ptr2->infos,ptr->infos,sizeof(SharedComponentInfo)*ptr->capacity);
                    ptr = ptr2;
                    dataChunk[typeIndex].reset(ptr2);
                }
            }
        }
    public:
        SharedComponentStore() = default;
        ~SharedComponentStore() {
        #if VERBOSE
            printf("~SharedComponentStore(): %p\n",this);
        #endif
            for (uint32_t i: range<uint32_t>(TypeID::MaximumTypesCount))
            {
                SharedComponentChunk *ptr = dataChunk[i].get();
                if(ptr && TypeManager::GetTypeInfoPointer()[i].TypeIndex.isManaged())
                {
                #if VERBOSE
                    printf("\tType %s:\n",TypeManager::GetTypeName(i));
                #endif
                    const uint32_t typesize = TypeManager::GetTypeInfoPointer()[i].TypeSize;
                    const SharedComponentInfo* infos = ptr->infos;
                    uint8_t *data = ptr->buffer;
                    for (uint32_t j = 0;j < ptr->count;j++)
                    {
                        if(infos[j].refCount > 0){
                            #if VERBOSE
                                //printf("\t\tValue %u %p, refCount \n", j, data, infos[j].refCount);
                            #endif
                            ((IManagedComponentData*)data)->~IManagedComponentData();
                        }
                        data += typesize;
                    }
                }
            }
        }
        void *getPointer(SharedComponentIndex index) noexcept {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                return nullptr;
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            if(ptr->infos[elementIndex].refCount < 1)
                return nullptr;
            return ptr->buffer + typeSize * elementIndex;
        }
        SharedComponentIndex allocate(TypeID type) {
            if(!type.isSharedComponent() || type.isZeroSized())
                throw std::bad_typeid();
            const uint32_t typeIndex = type.index();
            resizeIfNeeded(typeIndex);
            uint32_t index;
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            SharedComponentInfo *info;
            if(ptr->freeSlotIndex != SharedComponentChunk::InvalidIndex){
                index = ptr->freeSlotIndex;
                info = ptr->infos + index;
                ptr->freeSlotIndex = info->version;
                info->version = Version();
            }else{
                index = ptr->count++;
                info = ptr->infos + index;
                if(index > SharedComponentChunk::MaximumSize)
                    throw std::runtime_error("allocate(): allocating a new SharedComponent failed");
            }
            info->refCount = 1;
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            TypeManager::GetTypeInfoPointer()[typeIndex].defaultConstruct(ptr->buffer + typeSize * index);
            index |= typeIndex << SharedComponentIndex::TypeIndexBitOffset;
            return SharedComponentIndex{index};
        }
        void refIncrease(SharedComponentIndex index, uint32_t num = 1) {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            SharedComponentInfo *info = ptr->infos + elementIndex;
            if(num == 0)
                return;
            if(info->refCount == 0)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            // TODO overflow check.
            info->refCount += num;
        }
        void refDecrease(SharedComponentIndex index, uint32_t num = 1) {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refDecrease(): invalid shared component index");
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            SharedComponentInfo *info = ptr->infos + elementIndex;
            if(num == 0)
                return;
            if(info->refCount == 0 || info->refCount < num)
                throw std::runtime_error("refDecrease(): negative refrence count");
            info->refCount -= num;
            if(info->refCount == 0) {
                IManagedComponentData *com = (IManagedComponentData *)(ptr->buffer + typeSize * elementIndex);
                com->~IManagedComponentData();
                if(ptr->count == (elementIndex+1))
                    ptr->count--;
                else {
                    info->version = ptr->freeSlotIndex;
                    ptr->freeSlotIndex = elementIndex;
                }
            }
        }
    };
}

#endif
