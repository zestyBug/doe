#if !defined(RESOURCEMANAGER_HPP)
#define RESOURCEMANAGER_HPP

#include "cutil/basics.hpp"
#include "Base/TypeID.hpp"
#include "cutil/set.hpp"
#include <vector>

namespace ECS {
    struct ResourceEntry;
    class ResourceManager;

    class ResourceManager {
    public:
        typedef void(WaiterSignature)(void*,Resource);
    private:
        struct WaitNode {
            WaiterSignature *callback;
            void*        userData;
            WaitNode*    next;
        };
    private:
        class ResourceStore {
            struct ResourceChunk;
            ResourceID type;
            uint32_t typeSize = 0;
            uint32_t chunkCapacity = 0;
            uint32_t numAllocatedChunks = 0;
            ResourceEntry *freeEntriesTail = nullptr;
            ResourceEntry *pendingEntriesTail = nullptr;
            ResourceEntry *allEntriesTail = nullptr;
            set<ResourceEntry*> hashmap;
            std::unique_ptr<ResourceChunk> chunks[Constants::MaximumResourceChunkCount];
            void expand();
        public:
            void initialize(ResourceID);
            inline bool isInitialize() const {
                return this->typeSize != 0;
            }
            ResourceEntry& getOrCreate(Hash32);
            void free(ResourceEntry*);
            void garbageCollect();
        };
        ResourceStore stores[Constants::MaximumResourceTypeCount];
        ResourceStore *freeList = nullptr;
        WaitNode *avaialbleNode;
        WaitNode arena[Constants::ResourceManagerArenaSize];
        void destroy(ResourceEntry&);
    public:
        void loadResource(ResourceID id, string_view name, void *ctx, WaiterSignature *cb);
        void resourceReady(Resource r, bool success = true);
        void retain(Resource r);
        void release(Resource r);
        void* getResource(Resource r);
        ResourceEntry& validate(Resource r);
        void garbageCollect();

        ResourceManager(ResourceManager&&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;
        ResourceManager();
        ~ResourceManager();
    };
}

#endif // RESOURCEMANAGER_HPP
