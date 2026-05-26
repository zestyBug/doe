#include "ECS/ResourceManager.hpp"
using namespace ECS;
#pragma region ResourceManager
//ResourceManager::ResourceManager(){}
ResourceID ResourceManager::registerResource(LoadSignature *load, FreeSignature *free, void* ctx, uint32_t typeSize) {
    if(unlikely(typeCount >= Constants::MaximumResourcesCount))
        throw std::runtime_error("registerResource(): Constants::MaximumResourcesCount");
    if(!free || !load)
        throw std::invalid_argument("registerResource(): nullptr function");
    const uint32_t index = typeCount++;
    //sharedInfos[index].TypeIndex = ResourceID::fromIndex(index);
    //sharedInfos[index].TypeSize = sizeof(T);
    sharedValues[index] = std::make_unique<ResourceStore>();
    ResourceStore &store = *sharedValues[index];
    store.Load = load;
    store.Free = free;
    store.Ctx = ctx;
    store.typeSize = typeSize;
    store.type = index;
    //sharedInfos[index].Name = name;
    return index;
}
void ResourceManager::loadResource(ResourceID id, string_view name, void *ctx, WaiterSignature *cb)
{
    if(id >= ResourceManager::typeCount)
        throw std::invalid_argument("getStore(): invalid resource id");
    ResourceStore &store = *sharedValues[id];
    Hash32  hash = HashHelper::FNV1A32(name);
    int32_t index = store.hashmap.indexOf(hash);
    
    if(index < 0)
    {
        ResourceTask &task = store.allocateResource(hash);
        task.waiters.reserve(32);
        task.waiters.push_back(ResourceTask::Waiter{.ctx = ctx, .cb = cb});
        store.Load(store.Ctx,&task);
    }
    else
    {
        const uint32_t vindex = store.hashmap.getValue(index);
        ResourceTask &task = store.getResource(vindex);
        if(task.state == ResourceTask::ResourceState::Loaded)
            cb(ctx,task.res);
        else if(task.state == ResourceTask::ResourceState::Loading)
            task.waiters.push_back(ResourceTask::Waiter{.ctx = ctx, .cb = cb});
        else
            cb(ctx,Resource{.index = 0, .type = id});
    }
}
void* ResourceManager::getResource(Resource r){
    if(r.type >= ResourceManager::typeCount)
        throw std::invalid_argument("refCountIncrease(): invalid resource id");
    void *value = this->sharedValues[r.type]->getValue(r.index);
    return value;
}
void ResourceManager::refCountIncrease(Resource r){
    if(r.type >= ResourceManager::typeCount)
        throw std::invalid_argument("refCountIncrease(): invalid resource id");
    uint32_t &refCountRef = this->sharedValues[r.type]->getRefCount(r.index);
    if(refCountRef < 1)
        throw std::invalid_argument("refCountIncrease(): invalid resource");
    refCountRef++;
}
void ResourceManager::refCountDecrease(Resource r){
    if(r.type >= ResourceManager::typeCount)
        throw std::invalid_argument("refCountDecrease(): invalid resource id");
    ResourceStore &store = *this->sharedValues[r.type];
    uint32_t &refCountRef = store.getRefCount(r.index);
    if(refCountRef < 1)
        throw std::invalid_argument("refCountDecrease(): invalid resource");
    refCountRef--;
    if(refCountRef < 1)
    {
        ResourceTask &task = store.getResource(r.index);
        store.Free(store.Ctx, task);
        store.freeResource(task);
    }
}
#pragma endregion ResourceManager
#pragma region ResourceStore
ResourceTask& ResourceManager::ResourceStore::getResource(uint32_t index){
    if(this->hashmap.occupiedNodes() < index || index == 0)
        throw std::out_of_range("getBlock(): invalid index");
    return this->blocks[index / Constants::ResourceBlockSize]->resources[index % Constants::ResourceBlockSize];
}
uint32_t& ResourceManager::ResourceStore::getRefCount(uint32_t index){
    if(this->hashmap.occupiedNodes() < index || index == 0)
        throw std::out_of_range("getBlock(): invalid index");
    return this->blocks[index / Constants::ResourceBlockSize]->refCounts[index % Constants::ResourceBlockSize];
}
void* ResourceManager::ResourceStore::getValue(uint32_t index){
    if(this->hashmap.occupiedNodes() < index || index == 0)
        throw std::out_of_range("getBlock(): invalid index");
    uint8_t *block = this->blocks[index / Constants::ResourceBlockSize]->data;
    return block + this->typeSize * (index % Constants::ResourceBlockSize);
}
void ResourceManager::ResourceStore::addBlock(){
    if(Constants::ResourceBlockCount <= this->blockCount)
        throw std::runtime_error("addBlock(): can't expand");
    if(this->blocks[this->blockCount])
        throw std::runtime_error("addBlock(): can't expand");
    ResourceBlock *block = (ResourceBlock *)malloc(sizeof(ResourceBlock) + this->typeSize * Constants::ResourceBlockSize);
    new (block) ResourceBlock();
    this->blocks[this->blockCount].reset(block);
    for(uint32_t i=0;i<Constants::ResourceBlockSize;i++){
        block->resources[i].value = block->data + this->typeSize * i;
    }
    this->blockCount++;
}
ResourceTask& ResourceManager::ResourceStore::allocateResource(Hash32 hash)
{
    ResourceTask *res;
    uint32_t index_buffer;
    if(this->freeIndex != 0){
        index_buffer = this->freeIndex;
        this->hashmap.insert(hash, index_buffer);
        res = &this->getResource(index_buffer);
        this->freeIndex = res->res.index;
    }else{
        index_buffer = this->hashmap.occupiedNodes() + 1;
        this->hashmap.insert(hash, index_buffer);
        if(index_buffer >= Constants::ResourceBlockSize * this->blockCount)
            this->addBlock();
        res = &this->getResource(index_buffer);
    }
    this->getRefCount(index_buffer) = 1;
    res->res = Resource{.index = index_buffer, .type = this->type };
    res->hash = hash;
    return *res;
}
void ResourceManager::ResourceStore::freeResource(ResourceTask &task){
    this->hashmap.remove(task.hash);
    task.res.index = this->freeIndex;
    this->freeIndex = task.res.index;
    if(task.state == ResourceTask::ResourceState::Loading)
        throw std::runtime_error("freeResource(): destroying a pending resource");
    if(!task.waiters.empty())
        task.waiters.clear();
}
#pragma endregion ResourceStore