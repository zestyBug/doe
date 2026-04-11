#if !defined(MANAGEDCOMPONENTSTORE_HPP)
#define MANAGEDCOMPONENTSTORE_HPP

#include "cutil/range.hpp"
#include "cutil/HashHelper.hpp"
#include "Base/TypeID.hpp"
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
        align_ptr<align_ptr<SharedComponentChunk>[]> dataChunk;
        uint32_t sharedComponentVersion = 0;

        TypeID getComponentType(SharedComponentIndex value) {
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
        void initiate(uint32_t typeIndex){
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(unlikely(!ptr)){
                // count
                const uint32_t buffer1 =  32;
                // buffer size
                const uint32_t buffer2 = alignTo64(buffer1*typeSize);
                const uint32_t buffer3 = buffer2 + buffer1*sizeof(SharedComponentInfo);
                const uint32_t buffer4 = buffer3 + buffer1*sizeof(uint32_t);
                const uint32_t buffer5 = buffer4 + buffer1*sizeof(uint32_t);
                ptr = (SharedComponentChunk*) allocator().allocate(buffer5);
                ptr->capacity = buffer1;
                ptr->count = 1;
                ptr->freeSlotIndex = SharedComponentChunk::InvalidIndex;
                ptr->infos = (SharedComponentInfo*)(((uint8_t*)ptr)+buffer2);
                ptr->hashes =           (uint32_t*)(((uint8_t*)ptr)+buffer3);
                ptr->hashes_index =     (uint32_t*)(((uint8_t*)ptr)+buffer4);
                memset(ptr->infos, 0, buffer1*sizeof(SharedComponentInfo)+buffer1*sizeof(uint32_t));
                TypeManager::GetTypeInfoPointer()[typeIndex].defaultConstruct(ptr->buffer);
                uint32_t hashCode = HashHelper::FNV1A32(ptr->buffer,typeSize);
                if (hashCode == 0 || hashCode == SkipCode)
                    hashCode = AValidHashCode;
                ptr->infos[0].referenceCount = 1;
                ptr->infos[0].version = 0;
                ptr->infos[0].hash = hashCode;
                dataChunk[typeIndex].reset(ptr);
            }
        }
        void resize(uint32_t typeIndex){
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(unlikely(!ptr)){
                initiate(typeIndex);
            }else{
                // old capacity
                const uint32_t buffer0 = ptr->capacity;
                // the new capacity
                const uint32_t buffer1 = buffer0 * 2;
                SharedComponentChunk* ptr2;
            #if VERBOSE
                printf("SharedComponentStore::resize(): %u => %u\n", ptr->capacity, buffer1);
            #endif
                {
                    // buffer size
                    const uint32_t buffer2 = buffer1 * typeSize;
                    // plus info buffer size
                    const uint32_t buffer3 = buffer2 + buffer1*(uint32_t)sizeof(SharedComponentInfo);
                    // plus hash buffer size
                    const uint32_t buffer4 = buffer3 + buffer1*(uint32_t)sizeof(uint32_t);
                    // plus indecies buffer size
                    const uint32_t buffer5 = buffer4 + buffer1*(uint32_t)sizeof(uint32_t);
                    ptr2 = (SharedComponentChunk*) allocator().allocate(buffer5);
                    ptr2->capacity = buffer1;
                    ptr2->count = ptr->count;
                    ptr2->freeSlotIndex = ptr->freeSlotIndex;
                    ptr2->infos = (SharedComponentInfo*)(((uint8_t*)ptr2)+buffer2);
                    ptr2->hashes =           (uint32_t*)(((uint8_t*)ptr2)+buffer3);
                    ptr2->hashes_index =     (uint32_t*)(((uint8_t*)ptr2)+buffer4);
                }
                memcpy(ptr2->buffer,ptr->buffer,buffer0*typeSize);
                memcpy(ptr2->infos,ptr->infos,buffer0*sizeof(SharedComponentInfo));
                memset(ptr2->hashes, 0, buffer1*sizeof(uint32_t));
                uint32_t *indecies1 = ptr->hashes_index;
                uint32_t *indecies2 = ptr2->hashes_index;
                uint32_t *hashes1 = ptr->hashes;
                uint32_t *hashes2 = ptr2->hashes;
                for(uint32_t i=0;i<buffer0;i++){
                    const uint32_t hashBuffer = hashes1[i];
                    if(hashBuffer!=0 && hashBuffer!=SkipCode){
                        /** insertHash */ {
                            uint32_t offset = hashBuffer & (buffer1-1);
                            uint32_t attempts = 0;
                            while (true) {
                                if (hashes2[offset] == 0)
                                {
                                    hashes2[offset] = hashBuffer;
                                    indecies2[offset] = indecies1[i];
                                    break;
                                }
                                offset = (offset+1) & (buffer1-1);
                                ++attempts;
                                if(attempts == buffer1)
                                    throw std::runtime_error("resizeIfNeeded(): something went wrong");
                            }
                        }
                    }
                }
                ptr = ptr2;
                dataChunk[typeIndex].reset(ptr2);
            }
        }
    public:
        SharedComponentStore() {
            dataChunk = make_align<align_ptr<SharedComponentChunk>[]>(TypeID::MaximumTypesCount);
            memset(dataChunk.get(),0,TypeID::MaximumTypesCount*sizeof(align_ptr<SharedComponentChunk>));
        }
        ~SharedComponentStore() {
        #if VERBOSE
            printf("~SharedComponentStore(): %p\n",this);
        #endif
            for (uint32_t i: range<uint32_t>(TypeID::MaximumTypesCount))
            {
                SharedComponentChunk *ptr = dataChunk[i].get();
                if(ptr && TypeManager::GetTypeInfoPointer()[i].TypeIndex.isManagedComponent())
                {
                #if VERBOSE
                    printf("\tType %s:\n",TypeManager::GetTypeName(i));
                #endif
                    const uint32_t typesize = TypeManager::GetTypeInfo(i).TypeSize;
                    TypeManager::DefaultFunction dFunc = TypeManager::GetTypeInfo(i).defaultDestruct;
                    const SharedComponentInfo* infos = ptr->infos;
                    uint8_t *data = ptr->buffer;
                    for (uint32_t j = 0;j < ptr->count;j++)
                    {
                        if(infos[j].referenceCount > 0){
                            #if VERBOSE
                                //printf("\t\tValue %u %p, refCount \n", j, data, infos[j].refCount);
                            #endif
                            dFunc(data);
                        }
                        data += typesize;
                    }
                    dataChunk[i].reset();
                }
            }
            dataChunk.reset();
        }
        void *getPointer(SharedComponentIndex index) noexcept {
            if(index.isNull())
                return nullptr;
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            initiate(typeIndex);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                return nullptr;
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            if(ptr->infos[elementIndex].referenceCount < 1)
                return nullptr;
            return ptr->buffer + typeSize * elementIndex;
        }
        SharedComponentIndex insert(TypeID type,void *data) {
            if(!type.isSharedComponent() || type.isZeroSized())
                throw std::bad_typeid();
            if(data == nullptr)
                return SharedComponentIndex();
            const uint32_t typeIndex = type.index();
            initiate(typeIndex);

            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            uint32_t hashCode = HashHelper::FNV1A32(data,typeSize);
            if (hashCode == 0 || hashCode == SkipCode)
                hashCode = AValidHashCode;
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            SharedComponentInfo *info;
            uint32_t *hashes   = ptr->hashes;
            uint32_t *indecies = ptr->hashes_index;
            uint8_t  *buffer   = ptr->buffer;
            void *addressBuffer;
            uint32_t elementIndex;
            uint32_t capacity = ptr->capacity;
            uint32_t hashMask = capacity - 1;

            /** tryFind */{
                uint32_t offset = hashCode & hashMask;
                uint32_t attempts = 0;
                while (true)
                {
                    uint32_t hashBuffer = hashes[offset];
                    if (hashBuffer == 0)
                        break;
                    if (hashBuffer == hashCode)
                    {
                        elementIndex = indecies[offset];
                        addressBuffer = buffer + typeSize * elementIndex;
                        if (memcmp(addressBuffer,data,typeSize) == 0){
                            ptr->infos[elementIndex].referenceCount++;
                            goto finalize;
                        }
                    }
                    offset = (offset + 1) & hashMask;
                    ++attempts;
                    if (attempts == capacity)
                        break;
                }
            }
            if(ptr->freeSlotIndex != SharedComponentChunk::InvalidIndex){
                elementIndex = ptr->freeSlotIndex;
                info = ptr->infos + elementIndex;
                ptr->freeSlotIndex = (uint32_t)info->version;
            }else{
                elementIndex = ptr->count++;
                info = ptr->infos + elementIndex;
                if(elementIndex > SharedComponentChunk::MaximumSize)
                    throw std::runtime_error("allocate(): allocating a new SharedComponent failed");
                if(ptr->count == ptr->capacity)
                    resize(typeIndex);
            }
            info->referenceCount = 1;
            info->version = ++sharedComponentVersion;
            info->hash = hashCode;
            memcpy(ptr->buffer + typeSize * elementIndex, data, typeSize);
            /** insertHash */{
                uint32_t offset = hashCode & hashMask;
                uint32_t attempts = 0;
                while (true)
                {
                    uint32_t hashBuffer = hashes[offset];
                    if (hashBuffer == 0)
                    {
                        hashes[offset] = hashCode;
                        indecies[offset] = elementIndex;
                        break;
                    }
                    if (hashBuffer == SkipCode)
                    {
                        hashes[offset] = hashCode;
                        indecies[offset] = elementIndex;
                        break;
                    }
                    offset = (offset + 1) & hashMask;
                    ++attempts;
                    if(attempts == capacity)
                        throw std::runtime_error("insert(): something went wrong");
                }
            }
        finalize:
            elementIndex |= typeIndex << SharedComponentIndex::TypeIndexBitOffset;
            return SharedComponentIndex{elementIndex};
        }
        void addReference(SharedComponentIndex index, uint32_t num = 1) {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            SharedComponentInfo *info = ptr->infos + elementIndex;
            if(num == 0)
                return;
            if(info->referenceCount == 0)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            // TODO overflow check.
            info->referenceCount += num;
        }
        void removeReference(SharedComponentIndex index, uint32_t num = 1) {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refDecrease(): invalid shared component index");
            const uint32_t typeSize = TypeManager::GetTypeInfoPointer()[typeIndex].TypeSize;
            SharedComponentInfo *info = ptr->infos + elementIndex;
            if(num == 0)
                return;
            if(info->referenceCount == 0 || info->referenceCount < num)
                throw std::runtime_error("refDecrease(): negative reference count");
            info->referenceCount -= num;

            if(unlikely(info->referenceCount == 0)) {
                if(ptr->count == (elementIndex+1))
                    ptr->count--;
                else {
                    info->version = ptr->freeSlotIndex;
                    ptr->freeSlotIndex = elementIndex;
                }
                /** removeHash */{
                    uint32_t hashCode = info->hash;
                    uint32_t *hashes   = ptr->hashes;
                    uint32_t *indecies = ptr->hashes_index;
                    uint32_t capacity = ptr->capacity;
                    uint32_t hashMask = capacity - 1;
                    uint32_t offset = hashCode & hashMask;
                    uint32_t attempts = 0;
                    uint32_t hashBuffer;
                    while (true)
                    {
                        hashBuffer = hashes[offset];
                        if (hashBuffer == 0)
                            throw std::runtime_error("removeReference(): unable to find the hash");
                        if (hashBuffer == hashCode)
                            if(indecies[offset] == elementIndex){
                                hashes[offset] = SkipCode;
                                break;
                            }
                        offset = (offset + 1) & hashMask;
                        ++attempts;
                        if(attempts >= capacity)
                            // we should not reach here, a possiblyGrow() call must prevent it
                            throw std::runtime_error("insert(): something went wrong");
                    }
                }
                TypeManager::DefaultFunction dFunc = TypeManager::GetTypeInfo(typeIndex).defaultDestruct;
                dFunc(ptr->buffer + typeSize * elementIndex);
            }
        }
        void incrementVersion(SharedComponentIndex index) {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            SharedComponentInfo *info = ptr->infos + elementIndex;
            if(info->referenceCount == 0)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            info->version = ++this->sharedComponentVersion;
        }
        uint32_t getVersion(SharedComponentIndex index) const {
            const uint32_t typeIndex = getTypeIndex(index);
            const uint32_t elementIndex = getElementIndex(index);
            const SharedComponentChunk* ptr = dataChunk[typeIndex].get();
            if(!ptr || elementIndex >= ptr->count)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            const SharedComponentInfo *info = ptr->infos + elementIndex;
            if(info->referenceCount == 0)
                throw std::invalid_argument("refIncrease(): invalid shared component index");
            return info->version;
        }
        void checkInternalConsistency()
        {
            for (uint32_t i: range<uint32_t>(TypeID::MaximumTypesCount))
            {
                SharedComponentChunk *ptr = dataChunk[i].get();
                if(ptr)
                {
                    const uint32_t capacity = ptr->capacity;
                    const uint32_t hashMask = capacity - 1;
                    const SharedComponentInfo* infos = ptr->infos;
                    const uint32_t *hashes = ptr->hashes;
                    const uint32_t *indecies = ptr->hashes_index;
                    for (uint32_t elementIndex = 0;elementIndex < capacity;elementIndex++)
                    {
                        if(infos[elementIndex].referenceCount > 0)
                        /** tryFind */ {
                            const uint32_t hashCode = infos[elementIndex].hash;
                            uint32_t offset = hashCode & hashMask;
                            uint32_t attempts = 0;
                            while (true)
                            {
                                const uint32_t hashBuffer = hashes[offset];
                                if (hashBuffer == 0)
                                    throw std::runtime_error("CheckInternalConsistency(): Inconsistency");
                                if (hashBuffer == hashCode)
                                {
                                    if(elementIndex != indecies[offset])
                                        throw std::runtime_error("CheckInternalConsistency(): Inconsistency");
                                    break;
                                }
                                offset = (offset + 1) & hashMask;
                                ++attempts;
                                if (attempts == capacity)
                                    throw std::runtime_error("CheckInternalConsistency(): Inconsistency");
                            }
                        }
                    }
                }
            }
        }
    };
}

#endif
