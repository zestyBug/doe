#include "ECS/EntityCommandBuffer.hpp"
#include "ECS/Engine.hpp"
#include "ECS/Archetype.hpp"

ECS::EntityCommandBuffer::Header& ECS::EntityCommandBuffer::_allocate(uint32_t size) {
    if(unlikely(size<sizeof(Header)))
        throw std::invalid_argument("allocate: invalid size");
    uint32_t alignedSize = alignCacheLineSize(size);
    Node *node = this->tail;
    if (likely(node)) {
        uint32_t newPos = node->pos + alignedSize;
        if(unlikely(newPos > node->bodySize)){
            this->tail = newBlock(alignedSize);
            node->next.reset(this->tail);
            node = this->tail;
        }
    } else {
        node = this->tail = newBlock(alignedSize);
        this->head.reset(node);
    }
    Header *addr = (Header*)(node->body + node->pos);
    node->pos += alignedSize;
    return *addr;
}
ECS::EntityCommandBuffer::Header& ECS::EntityCommandBuffer::newCommand(uint32_t size){
    Header *addr = &this->_allocate(size);
    if(likely(this->lastCommand))
        this->lastCommand->next = addr;
    else
        this->firstCommand = addr;
    addr->commandSize = size;
    addr->prev = this->lastCommand;
    addr->next = nullptr;
    this->lastCommand = addr;
    return *addr;
}
ECS::EntityCommandBuffer::CreateEntityCommand& ECS::EntityCommandBuffer::newCommandCreate(){
    CreateEntityCommand *addr = (CreateEntityCommand *)&this->_allocate(sizeof(CreateEntityCommand));
    if(likely(this->lastCreate))
        this->lastCreate->next = addr;
    else
        this->firstCreate = addr;
    addr->commandId = Command::CreateEntity;
    addr->commandSize = sizeof(CreateEntityCommand);
    addr->prev = this->lastCreate;
    addr->next = nullptr;
    this->lastCreate = addr;
    return *addr;
}

ECS::EntityCommandBuffer::Node* ECS::EntityCommandBuffer::newBlock(uint32_t minsize) {
    uint32_t blockSize = Constants::EntityCommandBufferChunkSize - sizeof(Node);
    if (unlikely(minsize > blockSize))
        blockSize = minsize;
    Node *ret = (Node*) allocator().allocate(blockSize + sizeof(Node));
    new(ret) Node();
    ret->bodySize = blockSize;
    return ret;
}
void ECS::EntityCommandBuffer::replaceEntities(span<const Entity> cache,span<Entity> entities){
    for(Entity &e : entities){
        if(unlikely(!e.isValid())){
            e = cache.at(e.index());
        }
    }
}
void ECS::EntityCommandBuffer::setComponentValue(EntityComponentStore &ecs, span<const Entity> entities, TypeID type, span<const uint8_t> value){
    for(Entity e : entities){
        ecs.setComponentDataRawEntityHasComponent(e, type, value);
    }
}

void ECS::EntityCommandBuffer::playback(ECS::EntityComponentStore &ecs){
    Header *pos = this->firstCreate;
    std::unique_ptr<Entity[]> cache;
    if(this->entityCount)
    {
        cache = std::make_unique<Entity[]>(this->entityCount);
        uint32_t entityIndex = 0;
        while(pos){
            CreateEntityCommand &com = *(CreateEntityCommand*)pos;
            if(unlikely(com.commandId != Command::CreateEntity))
                throw std::runtime_error("playback(): expected create command command");
            if(!com.arch)
                com.arch = &ecs.getEmptyArchetype();
            if(unlikely((entityIndex + com.batchCount) > this->entityCount))
                throw std::runtime_error("playback(): unexpected entity count");

            auto shCom = com.arch->defaultSharedComponent();
            ecs.createEntities(*com.arch, {cache.get() + entityIndex, com.batchCount}, shCom);
            entityIndex += com.batchCount;
            pos = pos->next;
        }
        if(entityIndex != this->entityCount)
            throw std::runtime_error("playback(): unexpected entity count");
    }
    pos = this->firstCommand;
    while(pos){
        switch (pos->commandId)
        {
        case Command::DestroyEntities:
            {
                DestroyEntitiesCommand &com = *(DestroyEntitiesCommand*)pos;
                ecs.destroyEntities(com.entities);
                    throw std::runtime_error("playback(): entity delete right after creation");
            }
            break;

        case Command::AddComponent:
            {
                AddComponentCommand &com = *(AddComponentCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                ecs.addComponent(com.entities, com.type);
            }
            break;

        case Command::AddComponents:
            {
                AddComponentsCommand &com = *(AddComponentsCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                ecs.addComponents(com.entities,com.types);
            }
            break;
        
        case Command::AddComponentValue:
            {
                AddComponentWValueCommand &com = *(AddComponentWValueCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                ecs.addComponent(com.entities,com.type);
                setComponentValue(ecs, com.entities, com.type, com.value);
            }
            break;
        
        case Command::SetComponent:
            {
                SetComponentCommand &com = *(SetComponentCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                setComponentValue(ecs, com.entities, com.type, com.value);
            }
            break;
        
        case Command::RemoveComponent:
            {
                RemoveComponentCommand &com = *(RemoveComponentCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                ecs.removeComponent(com.entities,com.type);
            }
            break;
        case Command::RemoveComponents:
            {
                RemoveComponentsCommand &com = *(RemoveComponentsCommand*)pos;
                replaceEntities({cache.get(),this->entityCount}, com.entities);
                ecs.removeComponents(com.entities,com.types);
            }
            break;
        
        default:
            break;
        }
        pos = pos->next;
    }
}

ECS::Entity ECS::EntityCommandBuffer::createEntity(Archetype *arch){
    CreateEntityCommand *h = &this->newCommandCreate();
    h->commandId = Command::CreateEntity;
    h->batchCount = 1;
    h->arch = arch;
    return Entity(this->entityCount++, 0);
}

void ECS::EntityCommandBuffer::destroyEntity(span<const Entity> entities){
    if(unlikely(entities.empty()))
        throw std::invalid_argument("destroyEntity()");
    DestroyEntitiesCommand *h = (DestroyEntitiesCommand*)&this->newCommand((uint32_t)sizeof(DestroyEntitiesCommand) + entities.size_bytes());
    h->commandId = Command::DestroyEntities;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(DestroyEntitiesCommand)), entities.size()};
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
}

void ECS::EntityCommandBuffer::addComponent(span<const Entity> entities, TypeID type){
    if(unlikely(entities.empty()))
        throw std::invalid_argument("addComponent()");
    AddComponentCommand *h = (AddComponentCommand*)&this->newCommand((uint32_t)sizeof(AddComponentCommand) + entities.size_bytes());
    h->commandId = Command::AddComponent;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(AddComponentsCommand)), entities.size()};
    h->type      = type;
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
}

void ECS::EntityCommandBuffer::addComponents(span<const Entity> entities, span<const TypeID> types){
    if(unlikely(entities.empty() || types.empty()))
        throw std::invalid_argument("addComponents()");
    AddComponentsCommand *h = (AddComponentsCommand*)&this->newCommand((uint32_t)sizeof(AddComponentsCommand) + entities.size_bytes() + types.size_bytes());
    h->commandId = Command::AddComponents;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(AddComponentsCommand)), entities.size()};
    h->types     = {(TypeID*)((uint8_t*)h + sizeof(AddComponentsCommand) + entities.size_bytes()), types.size()};
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
    memcpy(h->types.data(),   types.data(),   entities.size_bytes());
}

void ECS::EntityCommandBuffer::addComponentValue(span<const Entity> entities, TypeID type, span<const uint8_t> value){
    if(unlikely(entities.empty() || !value.size_bytes()))
        throw std::invalid_argument("addComponentValue()");
    uint32_t size = TypeManager::GetTypeInfo(type).SizeInChunk;
    AddComponentWValueCommand *h = (AddComponentWValueCommand*)&this->newCommand((uint32_t)sizeof(AddComponentWValueCommand) + entities.size_bytes() + size);
    h->commandId = Command::AddComponentValue;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(AddComponentWValueCommand)), entities.size()};
    h->type      = type;
    h->value     = {          (uint8_t*)h + sizeof(AddComponentWValueCommand) + entities.size_bytes(), value.size_bytes()};
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
    memcpy(h->value.data(),   value.data(),   value.size_bytes());
}

void ECS::EntityCommandBuffer::setComponent(span<const Entity> entities, TypeID type, span<const uint8_t> value){
    if(unlikely(entities.empty() || !value.size_bytes()))
        throw std::invalid_argument("setComponent()");
    uint32_t size = TypeManager::GetTypeInfo(type).SizeInChunk;
    SetComponentCommand *h = (SetComponentCommand*)&this->newCommand((uint32_t)sizeof(SetComponentCommand) + entities.size_bytes() + size);
    h->commandId = Command::SetComponent;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(SetComponentCommand)), entities.size()};
    h->type      = type;
    h->value     = {          (uint8_t*)h + sizeof(SetComponentCommand) + entities.size_bytes(), value.size_bytes()};
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
    memcpy(h->value.data(),   value.data(),   value.size_bytes());
}

void ECS::EntityCommandBuffer::removeComponent(span<const Entity> entities, TypeID type){
    if(unlikely(entities.empty()))
        throw std::invalid_argument("removeComponent()");
    RemoveComponentCommand *h = (RemoveComponentCommand*)&this->newCommand((uint32_t)sizeof(RemoveComponentCommand) + entities.size_bytes());
    h->commandId = Command::RemoveComponent;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(RemoveComponentCommand)), entities.size()};
    h->type      = type;
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
}

void ECS::EntityCommandBuffer::removeComponents(span<const Entity> entities, span<const TypeID> types){
    if(unlikely(entities.empty() || types.empty()))
        throw std::invalid_argument("removeComponents()");
    RemoveComponentsCommand *h = (RemoveComponentsCommand*)&this->newCommand((uint32_t)sizeof(RemoveComponentsCommand) + entities.size_bytes() + types.size_bytes());
    h->commandId = Command::RemoveComponents;
    h->entities  = {(Entity*)((uint8_t*)h + sizeof(RemoveComponentsCommand)), entities.size()};
    h->types     = {(TypeID*)((uint8_t*)h + sizeof(RemoveComponentsCommand) + entities.size_bytes()), types.size()};
    memcpy(h->entities.data(),entities.data(),entities.size_bytes());
    memcpy(h->types.data(),   types.data(),   entities.size_bytes());
}