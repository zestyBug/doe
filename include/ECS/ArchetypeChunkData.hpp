#if !defined(ARCHTYPECHUNKDATA_HPP)
#define ARCHTYPECHUNKDATA_HPP

#include "cutil/span.hpp"
#include "basics.hpp"
#include "defs.hpp"

namespace DOTS
{
    struct ArchetypeChunkData final
    {
        // SOA for chunks in a archetype : index, shared component index, version, count
        uint8_t *data=nullptr;
        // maximum number of chunks information that can be stored, before grow
        uint32_t _capacity=0;
        // number of chunks allocated
        uint32_t _count=0;
        uint32_t componentCount=0;
        const uint32_t entityPerChunk=16;

        // version value: (suitable for single type iteration)
        // note all type are same in archetype structure
        //    [ type[0]: [chunk[0] ... chunk[_capacity]]
        //      type[...]:
        //      type[componentCount]: [chunk[0] ... chunk[_capacity]]

        uint32_t VOffset=0;

        friend class Archetype;

        ArchetypeChunkData(){
        }

        ArchetypeChunkData(const ArchetypeChunkData&)= delete;
        ArchetypeChunkData& operator = (const ArchetypeChunkData&)= delete;
        ArchetypeChunkData(ArchetypeChunkData&&)= default;

        void initialize(uint32_t _component_count) {
            componentCount=_component_count;
            if(_component_count < 1)
                throw std::invalid_argument("ArchetypeChunkData(): zero component archetype chunk data");
        }
        ~ArchetypeChunkData(){
            allocator().deallocate(this->data);
        }

    protected:
        Entity& _Entities(uint32_t index=0){
            return ((Entity*)(this->data))[index];
        }
        version_t& _ChangeVersion(uint32_t index=0){
            return ((version_t*)(this->data+VOffset))[index];
        }
    public:

        inline bool empty() const {
            return this->_count < 1;
        }
        inline uint32_t capacity() const {
            return this->_capacity;
        }
        inline uint32_t count() const {
            return this->_count;
        }

        inline span<Entity> getEntitiesArrayForChunk(uint32_t chunkIndex) {
            if(chunkIndex > this->_count)
                throw std::out_of_range("getChangeVersionArrayForType()");
            return span<Entity>(&_Entities(chunkIndex*entityPerChunk),entityPerChunk);
        }

        span<version_t> getChangeVersionArrayForType(uint32_t component_index)
        {
            if(component_index > this->componentCount)
                throw std::out_of_range("getChangeVersionArrayForType()");
            uint32_t index = component_index * this->_capacity;
            return span<version_t>(&_ChangeVersion(index),this->_count);
        }
        version_t& getChangeVersion(uint32_t component_index, uint32_t chunkIndex)
        {
            auto changeVersions = getChangeVersionArrayForType(component_index);
            return changeVersions.at(chunkIndex);
        }
        version_t& GetOrderVersion(uint32_t chunkIndex)
        {
            return getChangeVersion(0, chunkIndex);
        }
        void SetAllChangeVersion(uint32_t chunkIndex, version_t version)
        {
            if(chunkIndex > this->_count)
                throw std::out_of_range("SetAllChangeVersion()");
            for (uint32_t i = 1; i < this->componentCount; ++i)
                _ChangeVersion((i * this->_capacity) + chunkIndex) = version;
        }

        void popBack(){
            if(this->_count<1)
                throw std::out_of_range("removeAtSwapBack(): empty array");
            this->_count--;
        }


        /// @brief remove a chunk from array and fill it space with last chunk in the array if possible
        /// @param chunkIndex index of chunk to be deleted
        void removeAtSwapBack(uint32_t chunkIndex){
            if(this->_count<1 || chunkIndex >= this->_count)
                throw std::out_of_range("removeAtSwapBack(): out of range index");
            this->_count--;
            if (chunkIndex == this->_count)
                return;

            for (uint32_t i = 0; i < this->componentCount; i++)
                this->_ChangeVersion((i * this->_capacity) + chunkIndex) = this->_ChangeVersion((i * this->_capacity) + this->_count);
            memcpy(&_Entities(chunkIndex), &_Entities(_count),sizeof(Entity)*entityPerChunk);
        }

        /// @brief add at end of list
        /// @param count set it, if chunk contains entities
        void add(version_t version){
            uint32_t index = this->_count++;
            if(index>=this->_capacity)
                this->grow(this->_capacity<1 ? 2 : this->_capacity*2);

            // New chunk, so all versions are reset.
            for (uint32_t i = 0; i < this->componentCount; i++)
                this->_ChangeVersion((i * this->_capacity) + index) = version;
        }
        /// @brief append would be a more suitable name :)
        /// @param src source to be appended
        void moveChunks(ArchetypeChunkData& src){
            if(this->componentCount != src.componentCount ||
                this->entityPerChunk != src.entityPerChunk)
                throw std::invalid_argument("moveChunks(): src doesnt fit this object");
            if (this->_capacity < this->_count + src._count)
                this->grow(this->_count + src._count);

            memcpy(&_Entities(this->_count * entityPerChunk),
                    &src._Entities(),
                    sizeof(Entity) * entityPerChunk * src._count);

            memcpy(&this->_ChangeVersion(this->_count * this->componentCount),
                     &src._ChangeVersion(),
                     sizeof(version_t)* src._count * this->componentCount);

            this->_count += src._count;

            //! src._count = 0;
        }
        void grow(uint32_t new_capacity){
            if(new_capacity <= this->_capacity)
                throw std::invalid_argument("grow(): smaller new size");

            // new_capacity * size_in_byte_per_one_chunk

            //chunk
            const uint32_t new_e_size =    new_capacity * sizeof(Entity);

            //version
            const uint32_t new_v_size   =  new_capacity * sizeof(version_t) * this->componentCount;
            const uint32_t new_v_offset = new_e_size;


            uint8_t *new_data = allocator().allocate(new_v_size + new_v_offset);

            if(this->data != nullptr && this->_count > 0){
                memcpy(new_data,
                    &this->_Entities(),
                    this->_count * sizeof(Entity));

                for(uint32_t i = 0; i < componentCount; ++i) {
                    memcpy((version_t*)(new_data + new_v_offset) + (i * new_capacity),
                        &this->_ChangeVersion(i * this->_capacity),
                        this->_count * this->componentCount * sizeof(version_t));
                }
                allocator().deallocate(this->data);
            }

            this->data = new_data;
            this->_capacity = new_capacity;
            this->VOffset = new_v_offset;
        }
    };

} // namespace DOTS




#endif // ARCHTYPECHUNKDATA_HPP
