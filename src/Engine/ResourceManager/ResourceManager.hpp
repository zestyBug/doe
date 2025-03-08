#if !defined(RESOURCEMANAGER_HPP)
#define RESOURCEMANAGER_HPP

#include <vector>
#include <mutex>
#include <queue>
#include <atomic>
#include <stdint.h>
#include "flat_hash_map/unordered_map.hpp"
#include "cutil/gc_ptr.hpp"

namespace DOTS
{
    // precompile specified id
    using ResourceID = unsigned int;
    // runtime resource index key
    using ResourceKey = unsigned int;
    // a large number, any index greater than this considered as invalid index
    static constexpr ResourceKey invalidResource = 0x7fffffff;

    class Resource
    {
    private:
        ResourceKey key;
    public:
        operator ResourceKey(){return this->key;}
        Resource() = default;
        virtual ~Resource(){};
    };

    struct ResourceRequest {
        ResourceID id;
        void *ctx = nullptr;
        // if cb is set to null, it is a delete request, 
        // otherwise allocate request
        void (*cb)(void *,const Resource*) = nullptr;
    };
    
    /*
    each resouorce must have a unique id at runtime and only a unique name in compile time.
    id must begin with 0, there can be a limitation in number of id's
    id's must be recycled,
    resource manager is made of 2 lookup talbles: 
      table1["unique name"] => resource id
      table2[resource id]   => resource data
    if an id does not exists in the table1 means resource is not loaded currently.
    resource manager uses mark and sweep algorithm, and can be multithreaded as so:
      resource manager system add a mark job
       job creates array of markable for each thread, 
       that marking a resource proccess works as follow:
        mark_array[resource_id] = true;
      at the end all arrays are sumed up,
       and resource id's that was not being marked, are deleted from memory
      developer must specify which components and what properties should be counted, 
       mark_table will remaind same but the marking job must be modified.
    */
    class ResourceManager final
    {
    public:
    private:
        // contains index of a tombstone resource or invalidResource
        intptr_t free_id_index;
        struct resourceWrapper {
            Resource *value = nullptr;
            // id of resource
            ResourceID id;
            bool allocated = false;
            operator Resource& () {
                if(this->allocated)
                    return *this->value;
                return *(Resource *)nullptr;
            }
            resourceWrapper() = default;
            ~resourceWrapper(){
                if(this->allocated)
                    delete this->value;
                this->value = nullptr;
                this->allocated = false;
            };
        };
        // id to resource table
        std::vector<resourceWrapper> resource;
        ska::unordered_map<ResourceID,size_t> indexMap;

        ResourceKey create(ResourceID id){
            resourceWrapper *ref;
            size_t res;
            if(free_id_index < invalidResource){
                res = this->free_id_index;
                ref = &this->resource[res];
                this->free_id_index = (intptr_t)(ref->value);
            }else{
                res = this->resource.size();
                // TODO: res  < invalidResource
                ref = &this->resource.emplace_back();
            }
            // assert(res < invalidResource);
            this->indexMap.emplace(id,res);
            //ref->value = 
            ref->id = id;
            ref->allocated = true;
            return (ResourceKey)res;
        }
        
        // WARN: only used by garbage-collector
        void free(ResourceKey id) {
            if(id < this->resource.size() && this->resource[id].allocated){
                this->resource[id].allocated = false;
                delete this->resource[id].value;
                this->resource[id].value = (Resource*)this->free_id_index;
                this->free_id_index = id;
                this->indexMap.erase(this->resource[id].id);
            }
        }
    public:
        ResourceManager(/* args */):free_id_index(invalidResource){
            this->resource.reserve(32);
            this->indexMap.reserve(32);
        }
        ~ResourceManager(){
            //
        }
        bool has(ResourceID id) const {
            return this->indexMap.find(id) != this->indexMap.end();
        }
        ResourceKey get(ResourceID id) {
            ska::unordered_map<ResourceID,size_t>::iterator v = this->indexMap.find(id);
            if(v != this->indexMap.end()){
                return (ResourceKey)v->second;//this->resource[v->second].value;
            }else{
                return invalidResource;
            }
        }
        Resource* value(ResourceKey id){
            // TODO: verify key id
            if(id < this->resource.size())
                return this->resource[id].value;
            else
                return nullptr;
        }
    };
    
} // namespace DOTS


#endif // RESOURCEMANAGER_HPP
