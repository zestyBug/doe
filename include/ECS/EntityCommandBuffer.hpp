#if !defined(ENTITYCOMMANDBUFFER_HPP)
#define ENTITYCOMMANDBUFFER_HPP

#include "cutil/basics.hpp"
#include "Base/Entity.hpp"
#include "Base/TypeID.hpp"
#include "cutil/span.hpp"
#include <queue>

namespace ECS
{
    struct Archetype;
    struct EntityComponentStore;
    class EntityCommandBuffer final {
    protected:
        struct Node {
            align_ptr<Node> next{};
            uint32_t bodySize = Constants::CacheLineSize;
            uint32_t pos = 0;
            alignas(Constants::CacheLineSize) uint8_t body[0];
        };
        static_assert(sizeof(Node) == Constants::CacheLineSize);
        
        enum class Command : uint32_t {
            Invalid = 0,
            CreateEntity,
            DestroyEntities,
            AddComponent,
            AddComponents,
            AddComponentValue,
            SetComponent,
            RemoveComponent,
            RemoveComponents,
        };
        struct Header {
            Command commandId;
            uint32_t commandSize;
            Header *next;
            Header *prev;
        };
        struct CreateEntityCommand : Header {
            Archetype *arch;
            uint32_t batchCount;
        };
        struct DestroyEntitiesCommand : Header {
            span<Entity> entities;
        };
        struct AddComponentCommand : Header {
            span<Entity> entities;
            TypeID type;
        };
        struct AddComponentsCommand : Header {
            span<Entity> entities;
            span<TypeID> types;
        };
        struct AddComponentWValueCommand : Header {
            span<Entity> entities;
            TypeID type;
            span<uint8_t> value;
        };
        struct SetComponentCommand : Header {
            span<Entity> entities;
            TypeID type;
            span<uint8_t> value;
        };
        struct RemoveComponentCommand : Header {
            span<Entity> entities;
            TypeID type;
        };
        struct RemoveComponentsCommand : Header {
            span<Entity> entities;
            span<TypeID> types;
        };

        static_assert((sizeof(Node)+sizeof(Header)) < Constants::EntityCommandBufferChunkSize);

        align_ptr<Node> head;
        Header *firstCommand = nullptr;
        Header * lastCommand = nullptr;
        Header *firstCreate  = nullptr;
        Header * lastCreate  = nullptr;
        Node   *tail         = nullptr;
        uint32_t entityCount = 0;
        Header& _allocate(uint32_t size);
        Header& newCommand(uint32_t size);
        CreateEntityCommand& newCommandCreate();
        static Node* newBlock(uint32_t);
        static void replaceEntities(span<const Entity>,span<Entity>);
        static void setComponentValue(EntityComponentStore&, span<const Entity>, TypeID, span<const uint8_t>);
    public:
        /// @brief The entity must not have came from the EntityCommandBuffer::createEntity
        /// @param e 
        inline void destroyEntity(Entity e){destroyEntity({&e,1});}

        inline void addComponent (Entity e,                    TypeID type){addComponent({&e,1},type);}
        inline void addComponents(Entity e,                    span<const TypeID> types){addComponents({&e,1},types);}

        template<typename T>
        inline void addComponent(Entity e)                   {TypeID type = getTypeID<T>();addComponent({&e,1},type);}
        template<typename T>
        inline void addComponent(span<const Entity> entities){TypeID type = getTypeID<T>();addComponent(entities,type);}
        template<typename T>
        inline void addComponentValue(Entity e, const T& value){addComponentValue({&e,1},value);}
        template<typename T>
        void addComponentValue(span<const Entity> entities, const T& value){
            const TypeID id = getTypeID<T>();
            if constexpr (isSharedComponent<T>()){
                union manualManagement
                {
                    T _;
                    char ___;
                    manualManagement():___{0}{}
                    ~manualManagement(){}
                } stack;
                new(&stack._) T(value);
                addComponentValue(entities,id,{(uint8_t*)&stack._,sizeof(T)});
            } else {
                addComponentValue(entities,id,{(uint8_t*)&value,sizeof(T)});
            }
        }

        template<typename T>
        inline void setComponent(Entity e, const T& value){setComponent({&e,1},value);}
        template<typename T>
        void setComponent(span<const Entity> entities, const T& value){
            const TypeID id = getTypeID<T>();
            if constexpr (isSharedComponent<T>()){
                union manualManagement
                {
                    T _;
                    char ____;
                    manualManagement():____{0}{}
                    ~manualManagement(){}
                } stack;
                new(&stack._) T(value);
                setComponent(entities,id,{&stack._,sizeof(T)});
            } else {
                setComponent(entities,id,{&value,sizeof(T)});
            }
        }

        template<typename T> inline void removeComponent(Entity e)                    {TypeID type = getTypeID<T>();removeComponent({&e,1},type);}
        template<typename T> inline void removeComponent(span<const Entity> entities) {TypeID type = getTypeID<T>();removeComponent(entities,type);}
        inline void removeComponent (Entity e,  TypeID type)             {removeComponent({&e,1},type);}
        inline void removeComponents(Entity e, span<const TypeID> types) {removeComponents({&e,1},types);}


        void playback         (EntityComponentStore &);
        Entity createEntity   (Archetype* = nullptr);
        void destroyEntity    (span<const Entity> entities);
        void addComponent     (span<const Entity> entities, TypeID type);
        void addComponents    (span<const Entity> entities, span<const TypeID> types);
        void addComponentValue(span<const Entity> entities, TypeID type, span<const uint8_t> value);
        void setComponent     (span<const Entity> entities, TypeID type, span<const uint8_t> value);
        void removeComponent  (span<const Entity> entities, TypeID type);
        void removeComponents (span<const Entity> entities, span<const TypeID> types);

        void setName(Entity e, string_view name);
    };
}

#endif // ENTITYCOMMANDBUFFER_HPP