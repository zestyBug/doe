#include "ECS/ResourceManager.hpp"
// calculateChunkCapacity
#include "ECS/EntityComponentStore.hpp"
#include "ECS/Base/Chunk.hpp"
#include "cutil/set.hpp"
using namespace ECS;
#pragma region ResourceManager

struct alignas(16) ECS::ResourceEntry final {
    enum class ResourceState : uint16_t {
        Free = 0,
        Loading = 1,
        Loaded = 2,
        Failed = 3,
    };
    ResourceID type;
    uint32_t refCount = 0;
    uint16_t version;
    ResourceState state = ResourceState::Free;
    Hash32 hash = 0;
    void *data;
    void *waitList;
    void *user_ctx;
    ResourceEntry *next;
};

ResourceManager::ResourceManager(){
    this->avaialbleNode = &this->arena[0];
    for (size_t i = 1; i < Constants::ResourceManagerArenaSize; i++)
    {
        this->arena[i-1].next = &this->arena[i];
    }
    this->arena[Constants::ResourceManagerArenaSize-1].next = nullptr;
}
ResourceManager::~ResourceManager(){
}
void ResourceManager::loadResource(ResourceID id, string_view name, void *ctx, WaiterSignature *cb)
{
    const TypeManager::ResourceInfo &info = TypeManager::GetResourceInfo(id);
    ResourceStore &store = stores[id.value()];
    Hash32  hash = HashHelper::FNV1A32(name);
    if(unlikely(!store.isInitialize()))
        store.initialize(id);
    ResourceEntry &entry = store.getOrCreate(hash);
    Resource r;r.ptr = (void*)((intptr_t)(&entry) | entry.version);
    
    if(entry.state == ResourceEntry::ResourceState::Free 
        || entry.state == ResourceEntry::ResourceState::Loading)
    {
        WaitNode *node = this->avaialbleNode;
        if(unlikely(!node))
            throw std::runtime_error("getNode(): out of arena memory");
        this->avaialbleNode = node->next;
        node->callback = cb;
        node->userData = ctx;
        node->next = (WaitNode*)entry.waitList;
        entry.waitList = node;
    }
    else if(entry.state == ResourceEntry::ResourceState::Failed
        || entry.state == ResourceEntry::ResourceState::Loaded)
    {
        entry.refCount++;
        cb(ctx, r);
        entry.refCount--;
        if(entry.refCount < 1)
            destroy(entry);
    }else{
        throw std::runtime_error("loadResource(): unexpect resource state");
    }
    if(entry.state == ResourceEntry::ResourceState::Free)
    {
        entry.state = ResourceEntry::ResourceState::Loading;
        entry.refCount++;
        info.load(entry.data, info.ctx, r);
        entry.refCount--;
        if(entry.refCount < 1)
            destroy(entry);
    }
}
ResourceEntry& ResourceManager::validate(Resource r){
    ResourceEntry *entry = r.getAddress();
    if(!entry)
        throw std::invalid_argument("validate(): invalid resource");
    if(entry->version != r.getVersion())
        throw std::runtime_error("validate(): invalid resource");
    return *entry;
}
void* ResourceManager::getResource(Resource r){
    ResourceEntry &entry = validate(r);
    return entry.data;
}
void ResourceManager::retain(Resource r){
    ResourceEntry &entry = validate(r);
    entry.refCount++;
}
void ResourceManager::release(Resource r){
    ResourceEntry &entry = validate(r);
    if(entry.refCount < 1)
        throw std::runtime_error("release(): invalid resource");
    entry.refCount--;
    if(entry.refCount < 1)
        destroy(entry);
}
void ResourceManager::garbageCollect(){
    for (ResourceStore &store:stores)
    {
        store.garbageCollect();
    }
}
void ResourceManager::resourceReady(Resource r, bool success){
    ResourceEntry &entry = validate(r);
    if(entry.state != ResourceEntry::ResourceState::Loading)
        throw std::invalid_argument("resourceReady(): invalid resource");
    WaitNode *node = (WaitNode*)entry.waitList;
    while(node){
        node->callback(node->userData, r);
        if(node->next){
            node = node->next;
        }else{
            node->next = this->avaialbleNode;
            this->avaialbleNode = (WaitNode*) entry.waitList;
            entry.waitList = nullptr;
            break;
        }
    }
    if(success){
        entry.state != ResourceEntry::ResourceState::Loaded;
    }else{
        entry.state != ResourceEntry::ResourceState::Failed;
    }
    if(entry.refCount == 0)
        this->destroy(entry);
}
void ResourceManager::destroy(ResourceEntry &entry){
    ResourceStore &store = stores[entry.type.value()];
    store.free(&entry);
}
#pragma endregion ResourceManager
#pragma region ResourceStore

struct ECS::ResourceManager::ResourceStore::ResourceChunk {
    /// @details Considerations: must be cache line aligned
    static constexpr uint32_t MemoryOffset = Constants::CacheLineSize;
    /// @brief Maximum usable memory size
    static constexpr uint32_t BufferSize = Constants::ChunkSize - MemoryOffset;
    ResourceID type;
    void *dataOffset;
    alignas(MemoryOffset) mutable uint8_t buffer[BufferSize];
};
static_assert(sizeof(Chunk) == Constants::ChunkSize);
void ECS::ResourceManager::ResourceStore::initialize(ResourceID type){
    const TypeManager::ResourceInfo &info = TypeManager::GetResourceInfo(type);
    if(unlikely(info.typeSize < 4))
        throw std::runtime_error("expand(): unexpected resource size");
    uint16_t sizes[2];
    sizes[0] = sizeof(ResourceEntry);
    sizes[1] = info.typeSize;
    this->type = type;
    this->typeSize  = info.typeSize;
    this->chunkCapacity  = EntityComponentStore::calculateChunkCapacity({sizes,2},Chunk::BufferSize);
    if(unlikely(this->chunkCapacity < 2))
        throw std::runtime_error("expand(): unexpected resource size");
}
void ECS::ResourceManager::ResourceStore::expand(){
    if(unlikely(this->typeSize < 4 || this->chunkCapacity < 2 || 
        this->numAllocatedChunks >= Constants::MaximumResourceChunkCount))
        throw std::runtime_error("expand(): initialize first");

    const uint32_t offset = alignPointerSize(this->chunkCapacity * sizeof(ResourceEntry));
    ResourceChunk *addr = (ResourceChunk*)malloc(Constants::ChunkSize);
    this->chunks[this->numAllocatedChunks++].reset(addr);
    addr->dataOffset = addr->buffer + offset;
    memset(addr->buffer, 0, offset);

    {
        ResourceEntry *entries = (ResourceEntry*) addr->buffer;
        uint8_t       *datas   =                  addr->buffer + offset;
        for(uint32_t i = 0; i < this->chunkCapacity; i++)
        {
            ResourceEntry &entry = entries[i];
            entry.next = &entries[i+1];
            entry.data = datas + this->typeSize * i;
        }
       entries[this->chunkCapacity - 1].next = this->freeEntriesTail;
       this->freeEntriesTail = &entries[0];
    }
}
ECS::ResourceEntry& ECS::ResourceManager::ResourceStore::getOrCreate(Hash32 hash){
    int32_t index = this->hashmap.indexOf(hash);
    ResourceEntry *entry;
    if(index < 0) {
        entry = this->freeEntriesTail;
        if(unlikely(!entry)){
            this->expand();
            entry = this->freeEntriesTail;
            if(unlikely(!entry))
                throw std::runtime_error("getOrCreate(): expantion failed");
        }
        this->hashmap.insert(hash, entry);
        this->freeEntriesTail = entry->next;
        entry->type     = this->type;
        entry->refCount = 0;
        entry->version  = (entry->version + 1) & 0b1111;
        entry->state    = ResourceEntry::ResourceState::Free;
        entry->hash     = hash;
        entry->next     = nullptr;
        entry->waitList = nullptr;
    } else {
        entry = this->hashmap.getValue(index);
        if(unlikely(!entry))
            throw std::runtime_error("getOrCreate(): unexpected nullptr");
    }
    return *entry;
}
void ECS::ResourceManager::ResourceStore::free(ResourceEntry *entry){
    if(unlikely(!entry))
        throw std::invalid_argument("free(): nullptr");
    this->hashmap.remove(entry->hash);
    entry->next = this->pendingEntriesTail;
    entry->version = (entry->version + 1) & 0b1111;
    entry->refCount = 0;
    this->pendingEntriesTail = entry;
}
void ECS::ResourceManager::ResourceStore::garbageCollect(){
    const TypeManager::ResourceInfo &info = TypeManager::GetResourceInfo(this->type);
    ResourceEntry *entry = this->pendingEntriesTail;
    while (entry)
    {
        Resource r;r.ptr = (void*)((intptr_t)(entry) | entry->version);
        info.free(entry->data, info.ctx, r);
        entry->state = ResourceEntry::ResourceState::Free;
        if(unlikely(entry->next == nullptr)){
            entry->next = this->freeEntriesTail;
            this->freeEntriesTail = this->pendingEntriesTail;
            this->pendingEntriesTail = nullptr;
            break;
        }else
            entry = entry->next;
    }
}
#pragma endregion ResourceStore