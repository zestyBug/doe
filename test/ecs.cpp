#include <vector>
#include <array>
#include <bitset>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <queue>
#include <memory>

#include <thread>
#include <condition_variable>
#include <mutex>

template<typename T, size_t S>
class StaticArray
{
    using Type = T;
private:
    union container
    {
        Type array[S];
        bool _;
        container(){
            _ = false;
        }
    };
    container _data;
    size_t count = 0;
public:
    static_assert(S > 0);
    StaticArray(){}
    ~StaticArray(){}
    inline size_t capacity()const {return S;}
    inline size_t size()const {return this->count;}
    inline bool   empty()const {return this->count < 1;}
    inline bool   full()const {return this->count == S;}
    inline Type*  begin(){return this->_data.array;}
    inline Type*  end(){return this->_data.array + S;}
    inline const Type* begin()const {return this->_data.array;}
    inline const Type* end()const {return this->_data.array + S;}
    inline T* data ()const {return this->_data.array;}
    template<typename ... Args>
    inline void emplace(Args ... arg){
        assert(!this->full());
        new (&(this->_data.array[this->count++])) Type(arg...);
    }
    inline void push(const Type& v){
        assert(!this->full());
        new (&(this->_data.array[this->count++])) Type(v);
    }
    inline void pop_back() {
        if(this->count > 0){
            this->count--;
            this->_data.array[this->count].~Type();
        }
    }
    inline Type& operator[](size_t index) {
        assert(S > index);
        return this->_data.array[index];
    }
    inline const Type& operator[](size_t index) const {
        assert(S > index);
        return this->_data.array[index];
    }
    inline void pop(size_t index) {
        if(this->count > index)
        {
            this->_data.array[index].~Type();
            this->count--;
            if(this->count > 0 && this->count != index)
                memcpy(this->_data.array + index, this->_data.array + this->count, sizeof(Type));
        }
    }
    // pop without filling the created empty space in middle of array
    inline void pop2(size_t index) {
        if(this->count > index)
            this->_data.array[index].~Type();
    }
};

namespace DOTS
{
    // type id or type bitmask
    using compid_t = uint32_t;

    struct comp_info {
        // a unique id, starts from 0
        compid_t index;
        size_t size;
        // destructor function
        void (*destructor)(void*);
    };
    // contains runtime typeinfo, 
    // technically array can be erased to reassign type ids 
    // unless values are stored somewhere and spreaded
    // WARN: dont access this directlys
    extern StaticArray<comp_info,32> rtti;

    // entity_t >> 24: version or archtype index, depending on situation
    // entity_t & 0xffffff: index in Register::entity_value  or Archtype::components, depending on situation
    using entity_t = uint32_t;
    using Entity = uint32_t;
    // index of archtype in archtype list
    using archtypeId_t = uint32_t;
    

    inline size_t get_index(entity_t e){
        return e & 0xffffff;
    }
    inline Entity get_version(Entity e){
        return e & 0xff000000;
    }
    inline size_t get_archtype_index(entity_t e){
        return (e & 0xff000000) >> 24;
    }
    inline size_t get_archtype(entity_t e){
        return e & 0xff000000;
    }

    [[nodiscard]] comp_info _new_id(size_t size, void (*destructor)(void*)) noexcept
    {
        comp_info info{(compid_t)rtti.size(), size, destructor};
        rtti.push(info);
        return info;
    }

    // actuall core of compile-time-type-information
    template<typename T>
    [[nodiscard]] comp_info __type_id__() 
    {
        static const comp_info value = _new_id(sizeof(T), [](void* x){static_cast<T*>(x)->~T();});
        return value;
    }

    // type can be specialized to fix a type id
    template<typename T>
    [[nodiscard]] inline comp_info type_id() 
    {
        return __type_id__<std::remove_const_t<std::remove_reference_t<T>>>();
    }
    [[nodiscard]] inline comp_info type_id(const compid_t id) 
    {
        return rtti[id];
    }

    // a bitmask
    template<typename T>
    [[nodiscard]] inline compid_t type_bit() 
    {
        return 1 << type_id<T>().index;
    }













    struct Archtype final {
        std::array<void*,33> components;
        std::array<comp_info,33> components_info;
        size_t capacity;
        size_t size;
        
        Archtype():capacity(0),size(0){
            memset(&this->components,0,sizeof(this->components));
        };

        void initialize(compid_t types_bitmask,const size_t initSize=4) {
            this->capacity = initSize;
            assert(this->capacity > 0);

            for(int bitmask=1,i=0;i < (int)(sizeof(types_bitmask)*8);i++){
                if(types_bitmask & bitmask)
                    initialize_component(i);
                bitmask <<= 1;
            }
            printf("Debug: malloc(%llu)\n",this->capacity*sizeof(Entity));
            this->components[32] = malloc(this->capacity*sizeof(Entity));
            this->components_info[32] = comp_info{32,sizeof(Entity),nullptr};
        }

        void initialize_component(const compid_t type) {
            const comp_info info = type_id(type);
            const size_t s = info.size * this->capacity;
            printf("Debug: malloc(%llu)\n",s);
            this->components[info.index] = malloc(s);
            this->components_info[info.index] = info;
        }

        // recives id of entity (index of entity in registers enity array + version)
        // returns a new valid index in this archtype
        entity_t allocate(const Entity index) {
            assert(this->size < 0xffffff);
            if(this->size < this->capacity){
                ((entity_t*)this->components[32])[this->size] = index;
                return this->size++;
            }else{
                for (size_t i = 0; i < 33; i++)
                    if(components[i]) {
                        const size_t old_size = this->components_info[i].size * this->capacity;
                        printf("Debug: realloc(%llu >> %llu)\n",old_size,old_size*2);
                        assert(this->components[i] = realloc(this->components[i],old_size*2));
                    }
                this->capacity *= 2;
                // TODO: covers wierd scenarios
                return allocate(index);
            }
        }

        // recives index in components array
        // returns id of entity that filled the empty space in array or invalid index
        Entity destroy(const Entity index) {
            assert(this->size > index);
            this->size--;
            for (size_t i = 0; i < 33; i++)
                if(components[i]){
                    const comp_info info = this->components_info[i];
                    char * const ptr = (char*)(components[i]) + (index * info.size);
                    if(info.destructor)
                       info.destructor(ptr);
                    if(index != this->size){
                        char * const last = (char*)(this->components[i]) + (this->size * info.size);
                        memcpy(ptr,last,info.size);
                    }
                }
            if(index != this->size){
                return ((entity_t*)this->components[32])[index];
            }else{
                return 0xffffff;
            }
        }
        // same as destroy() without calling destructor functions
        Entity destroy2(const Entity index) {
            assert(this->size > index);
            this->size--;
            const size_t size_buffer = this->size;
            if(index != size_buffer)
                for (size_t i = 0; i < 33; i++)
                    if(char * const ptr = (char*)(this->components[i]);ptr){
                            const size_t comonent_size = this->components_info[i].size;
                            memcpy(ptr+(index*comonent_size) , ptr+(size_buffer*comonent_size) , comonent_size);
                    }
            if(index != size_buffer){
                return ((entity_t*)this->components[32])[index];
            }else{
                return 0xffffff;
            }
        }
        // destroy all entities in this given archtype and itself
        void destroy(){
            for (size_t comp = 0; comp < 33; comp++)
                if(this->components[comp] != nullptr) {
                    printf("Debug: free(%llu)\n",this->components_info[comp].size * this->capacity);
                    if(this->components_info[comp].destructor)
                        for (size_t i = 0; i < this->size; i++)
                        {
                            void *ptr = (char*)(this->components[comp]) + (this->components_info[comp].size * i);
                            this->components_info[comp].destructor(ptr);
                        }
                    free(this->components[comp]);
                }
            memset(this->components.data(),0,sizeof(this->components));
            this->size=0;
            this->capacity=0;
        }
        ~Archtype(){
            this->destroy();
        }
    };





    struct DataChunk final {
        size_t count;
        Entity *entity;
        void ** component;
    };

    class System {
        friend class Register;
    protected:
        virtual void Start(){};
        virtual void Update(){};
        virtual void Stop(){};
    public:
        System(){}
        virtual ~System(){}
    };









    class Register final {
        static const entity_t null_entity_id = 0xffffff;
        static const archtypeId_t null_archtype_index = 0xffffffff;
        // array of entities value,
        // contains index of it archtype and it index in that archtype 
        std::vector<entity_t> entity_value;
        std::array<Archtype,256> archtypes;
        // contains bitmask of archtype, 
        // contains something else if archtype is not allocated and size/capacity is set to 0
        std::array<compid_t,256> archtypes_id;
        // contains index of last archtype, acts as size of archtypes_id and archtypes array
        archtypeId_t archtypes_index = 0;
        std::vector<std::unique_ptr<System>> system;

        entity_t free_entity_index = null_entity_id;
        archtypeId_t free_archtype_index = null_archtype_index;

        // find or create a archtype with given types, 
        // NOTE: without initializing it
        archtypeId_t getArchtypeIndex(compid_t comp_bitmap){
            archtypeId_t archtype_index;

            for(size_t i = 0;i < this->archtypes_index;i++){
                if(this->archtypes_id[i] == comp_bitmap && this->archtypes[i].capacity != 0){
                    return i;
                }
            }

            if(free_archtype_index == null_archtype_index){
                assert(this->archtypes_index < this->archtypes_id.size());
                archtype_index = this->archtypes_index;
                this->archtypes_index++;
            }else{
                archtypeId_t next_index = this->archtypes_id[this->free_archtype_index];
                /// TODO: may a invalid index check be good
                archtype_index = this->free_archtype_index;
                this->free_archtype_index = next_index;
            }
            this->archtypes_id[archtype_index] = comp_bitmap;
            this->archtypes[archtype_index].initialize(comp_bitmap);
            printf("Debug: new archtype bitmask %u\n",comp_bitmap);
            return archtype_index;
        }
        // finds a empty index on entity array and return it index + version, 
        // WARN: it may containts invalid value
        Entity createEntity(){
            Entity result;
            if(free_entity_index == null_entity_id){
                size_t index = entity_value.size();
                assert(index < 0xffffff);
                entity_value.push_back(null_entity_id);
                result = index;
            }else{
                const entity_t prev_val = entity_value[free_entity_index];
                unsigned int e_version = get_version(prev_val);
                result = free_entity_index | (e_version + 0x1000000);
                free_entity_index = get_index(prev_val);
            }
            printf("Debug: new entity %u\n",result);
            return result;
        }
        // destroy component if empty otherwise nothing
        void destroyEmptyComponent(const archtypeId_t archtype_index){
            if(this->archtypes[archtype_index].size == 0) {
                printf("Debug: destroy archtype bitmask %u\n",this->archtypes_id[archtype_index]);
                this->archtypes_id[archtype_index] = free_archtype_index;
                free_archtype_index = archtype_index;
                this->archtypes[archtype_index].destroy();
            }
        }
        // same as destroyComponents() without calling destructor
        void destroyComponents2(entity_t value){
            archtypeId_t archtype_index = get_archtype_index(value);
            entity_t index = get_index(value);
            const entity_t modified = get_index( this->archtypes[archtype_index].destroy2(index) );
            // if it index was filled with other entity
            if(modified != null_entity_id)
                this->entity_value[modified] = index | (archtype_index<<24);
            // otherwise either archtype was empty or entity was indexed last one in array
            destroyEmptyComponent(archtype_index);
        }
        // removes comonents not entity itsel, recives entity value (index + archtype)
        void destroyComponents(entity_t value){
            archtypeId_t archtype_index = get_archtype_index(value);
            entity_t index = get_index(value);
            const entity_t modified = get_index( this->archtypes[archtype_index].destroy(index) );
            // if it index was filled with other entity
            if(modified != null_entity_id)
                this->entity_value[modified] = index | (archtype_index<<24);
            // otherwise either archtype was empty or entity was indexed last one in array
            destroyEmptyComponent(archtype_index);
        }
        // charges archtype, recives entity id and components bitmap
        void _changeComponent(Entity entity, compid_t new_component_bitmap){
            compid_t old_component_bitmap;
            const entity_t entity_index = get_index(entity);
            assert(entity_index < this->entity_value.size());
            entity_t old_value = this->entity_value[entity_index];
            const entity_t old_value_index = get_index(old_value);
            // WARN: empty entities contains invalid archtype id
            const archtypeId_t old_archtype_index = get_archtype_index(old_value);

            if(old_value_index == null_entity_id){
                old_component_bitmap = 0;
            }else{
                old_component_bitmap = this->archtypes_id[old_archtype_index];
            }
            
            if(old_component_bitmap == new_component_bitmap){// nothing to be done
            }else if(new_component_bitmap == 0){// change to no component
                if(old_component_bitmap != 0){
                    // remove all components
                    destroyComponents(old_value);
                }
                this->entity_value[entity_index] = null_entity_id;
            }else{
                const archtypeId_t new_archtype_index = getArchtypeIndex(new_component_bitmap);
                const entity_t new_value_index = this->archtypes[new_archtype_index].allocate(entity);
                this->entity_value[entity_index] = new_value_index | (new_archtype_index << 24);
                // literally creates a new entity
                if(old_component_bitmap == 0){
                }else{ // moving old components
                    for (compid_t i = 0; i < 32; i++){
                        if(old_component_bitmap & 1) {
                            const comp_info component_info = type_id(i);
                            // TODO: does *_component_bitmap means we can access component array blindly? i mean without null pointer check.
                            void *src = ((char*)(this->archtypes[old_archtype_index].components[i])) + (old_value_index * component_info.size);
                            if(new_component_bitmap & 1){
                                void *dst = ((char*)(this->archtypes[new_archtype_index].components[i])) + (new_value_index * component_info.size);
                                memcpy(dst,src,component_info.size);
                            }else{
                                if(component_info.destructor)
                                    component_info.destructor(src);
                            }
                        }
                        old_component_bitmap >>= 1;
                        new_component_bitmap >>= 1;
                    }
                    destroyComponents2(old_value);
                }
            }
        }

    public:
        Register(){};
        ~Register(){};
        
        // create empty entity
        Entity create(){
            Entity result = createEntity();
            entity_value[get_index(result)] = null_entity_id;
            return result;
        }
        // create entity with given Components
        template<typename ... Args>
        Entity create() {
            compid_t comp_bitmap = (type_bit<Args>() | ...);

            const archtypeId_t archtype_index = getArchtypeIndex(comp_bitmap);

            const Entity result = createEntity();
            const entity_t value = this->archtypes[archtype_index].allocate(result) | (archtype_index << 24);
            this->entity_value[get_index(result)] = value;
            return result;
        }
        // destroy a entity entirly
        void destroy(Entity e) {
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t old_value = this->entity_value[index];

            this->entity_value[index] = free_entity_index | get_version(e);
            free_entity_index = index;

            // entity is empty
            if(get_index(old_value) == null_entity_id)
                return;

            destroyComponents(old_value);
        }

        template<typename T>
        void addComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            entity_t old_value = this->entity_value[index];
            compid_t components_bitmap = type_bit<T>();

            // empty entities may contain invalid component id
            if(get_index(old_value) != null_entity_id){
                const archtypeId_t old_archtype_index = get_archtype_index(old_value);
                components_bitmap |= this->archtypes_id[old_archtype_index];
            }

            _changeComponent(e,components_bitmap);
        }
        template<typename T>
        void removeComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t old_value = this->entity_value[index];
            compid_t new_component_bitmap=0;

            if(get_index(old_value) != null_entity_id){
                const archtypeId_t archtype_index = get_archtype_index(old_value);
                new_component_bitmap = this->archtypes_id[archtype_index] & ~type_bit<T>();
            }

            _changeComponent(e,new_component_bitmap);
        }
        template<typename T>
        auto& getComponent(Entity e) const {
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t value = this->entity_value[index];
            const archtypeId_t archtype_index = get_archtype_index(value);

            // empty entity
            assert(get_index(value) != null_entity_id);
            // component does not exists
            assert(this->archtypes[archtype_index].components[type_id<T>().index]);
            return ((T*)(this->archtypes[archtype_index].components[type_id<T>().index]))[get_index(value)];
        }
        template<typename T>
        bool hasComponent(Entity e){
            const entity_t index = get_index(e);
            assert(index < this->entity_value.size());
            const entity_t value = this->entity_value[index];

            // no component
            if(get_index(value) == null_entity_id)
                return false;
            return this->archtypes_id[get_archtype_index(value)] & type_bit<T>();
        }
        // TODO: recives a chunk size, so gives call the callback multiple times 
        // with array of pointers to components plus entity id list and maximum size of arrays as arguments
        template<typename ... Types>
        void iterate(void(*func)(std::array<void*, sizeof...(Types) + 1>,size_t), const size_t chunk_size = 16) {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            const compid_t comps_index[sizeof...(Types)] = {(type_id<Types>().index)...};
            const size_t   comps_size[sizeof...(Types)] = {(type_id<Types>().size)...};
            std::array<void*, sizeof...(Types) + 1> args;
            for (archtypeId_t i = 0; i < this->archtypes_index; i++)
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    Archtype &arch = this->archtypes[i];
                    if(arch.size != 0)
                        for(size_t chunck_index=0; chunck_index < arch.size; chunck_index+=chunk_size) {
                            size_t comp_index = 0;
                            for (; comp_index < sizeof...(Types); comp_index++)
                                args[comp_index] = ((char*)(arch.components[comps_index[comp_index]])) + comps_size[comp_index] * chunck_index;
                            args[comp_index] = ((entity_t*)(arch.components[32])) + chunck_index;
                            const size_t remaind = arch.size - chunck_index;
                            func(args,remaind<chunk_size?remaind:chunk_size);
                        }
                }
        }

        template<typename Type>
        Type template_wrapper(std::array<void*,33> &comps,size_t entity_index){
            return ((std::add_pointer_t<std::remove_const_t<std::remove_reference_t<Type>>>)comps[type_id<Type>().index]) [entity_index];
        }

        // TODO: recives a chunk size, so gives call the callback with multiple times 
        // with array of pointers to components and maximum size of arrays as argument
        template<typename ... Types>
        void iterate(void(*func)(Entity,Types...)) {
            const compid_t comps_bitmap = (type_bit<Types>() | ...);
            for (archtypeId_t i = 0; i < this->archtypes_index; i++)
                if((this->archtypes_id[i] & comps_bitmap) == comps_bitmap){
                    // arch might be empty and unallocatted
                    Archtype &arch = this->archtypes[i];
                    for(size_t entity_index=0; entity_index < arch.size; entity_index++)
                        func( ((Entity*)arch.components[32])[entity_index], template_wrapper<Types>(arch.components,entity_index) ...);
                }
        }
        template<typename Type>
        void addSystem(){
            this->system.emplace_back(new Type());
        }
        void executeSystems() {
            for(auto& i:this->system)
                i->Update();
        }
    };



    struct Job{
        entity_t (*next)(entity_t);
        void (*proc)(entity_t,entity_t);
    };

    class semaphore {
        std::mutex mutex_;
        std::condition_variable condition_;
        volatile unsigned long count_ = 0; // Initialized as locked.

    public:
        semaphore(unsigned long initial_value = 0) : count_(initial_value) {}
        // signal
        void release() {
            std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
            ++this->count_;
            this->condition_.notify_one();
        }
        // wait
        void acquire() {
            std::unique_lock<decltype(this->mutex_)> lock(this->mutex_);
            while (count_ == 0) // Handle spurious wake-ups.
                this->condition_.wait(lock);
            --this->count_;
        }
        // try wait()
        bool try_acquire() {
            std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
            if (this->count_ != 0) {
                --this->count_;
                return true;
            }
            return false;
        }
        void set_value(const unsigned long value){
             std::lock_guard<decltype(this->mutex_)> lock(this->mutex_);
            this->count_ = value;
        }
    };

    class ThreadPool final {
        // need to keep track of threads so we can join them
        std::vector<std::thread> worker;
        // each group has it own task queue
        std::vector<std::vector<Job>> group;


        semaphore finished;
        std::mutex gmutex;
        std::condition_variable barrier;

        volatile bool destroy = false;
        volatile unsigned int waiting_threads = 0;
        volatile unsigned int group_index = 0;
        volatile unsigned int job_index = 0;
        volatile entity_t entity_index = 0;

        void func(){while(true){
                std::unique_lock lock(this->gmutex);

                unsigned int waiting_threads_buffer = this->waiting_threads;
                const unsigned int group_index_buffer = this->group_index;
                unsigned int job_index_buffer = this->job_index;


                if(this->destroy)
                    return;
                    
                //reached end of groups
                if(group_index_buffer >= this->group.size())
                    goto sleeping_finished;
                
                // this group is empty (barrier)
                else if(this->group[group_index_buffer].size() == 0)
                    goto sleeping;
                
                // checkin  validity of job_index_buffer is not required:
                // 1-It begins with 0, and we have checked that group size is not zero
                // 2-In proccess of increament at the end of every job, validity is checked.
                else {

                    Job& j = this->group[group_index_buffer][job_index_buffer];
                    const entity_t entity_index_buffer1 = this->entity_index;
                    const entity_t entity_index_buffer2 = j.next(entity_index_buffer1);

                    // reached end of a job
                    if(entity_index_buffer2 == entity_index_buffer1) {
                        job_index_buffer++;
                        // validity check remainding job in group
                        if(job_index_buffer >= this->group[group_index_buffer].size()){
                            // goto next group.
                            this->group_index=group_index_buffer+1;
                            this->job_index = 0;
                        }else{
                            this->job_index=job_index_buffer;
                        }
                        this->entity_index = 0;
                    }else{
                        this->entity_index = entity_index_buffer2;
                        lock.unlock();
                        j.proc(entity_index_buffer1,entity_index_buffer2);
                        // do proccess here
                    }
                }
                continue;
            sleeping:
                if(++waiting_threads_buffer < this->worker.size()){
                    goto normal_thread_sleep;
                }else{
                    this->waiting_threads = 0;
                    this->group_index=group_index_buffer+1;
                    this->job_index = 0;
                    this->entity_index = 0;
                    this->barrier.notify_all();
                }
                continue;
            sleeping_finished:
                if(++waiting_threads_buffer >= this->worker.size())
                    this->finished.release();
            normal_thread_sleep:
                this->waiting_threads=waiting_threads_buffer;
                this->barrier.wait(lock);
                continue;
        }}
    public:
        ThreadPool(const uint32_t thread_count)
            :worker(thread_count),finished(0)
        {
            std::lock_guard lock(this->gmutex);
            for(auto& t: this->worker)
                t = std::thread{&ThreadPool::func,this};
            group.reserve(64);
        }
        void wait(){
            this->finished.acquire();
            this->group.clear();
        }
        void restart(){
            std::lock_guard lock(this->gmutex);
            this->waiting_threads = 0;
            this->group_index = 0;
            this->job_index = 0;
            this->entity_index = 0;
            this->barrier.notify_all();
        }
        void addJob(const Job& j,size_t group_id){
            if(this->group.size() <= group_id)
                this->group.resize(group_id+1);
            this->group[group_id].push_back(j);
        }
        ~ThreadPool(){
            destroy = true;
            this->barrier.notify_all();
            for(auto& t: this->worker)
                t.join();
            this->worker.clear();
        }
    };
}

StaticArray<DOTS::comp_info,32> DOTS::rtti;


void f1(DOTS::Entity, int& v1){
    v1 = 69;
}

void f2(DOTS::Entity, int v1){
    printf("%d\n",v1);
}

class TransformSystem : public DOTS::System {
    void Update(){
        printf("nuriiiii!\n");
    }
public:
    TransformSystem(){
        printf("nuriiiii!\n");
    }
};

int main(){
    DOTS::Register *reg = new DOTS::Register();
    /*
    auto v1 = reg->create<int,float,bool>();
    reg->create<int,float,bool>();
    reg->create<int,float>();
    reg->getComponent<int>(v1) = 10;
    reg->create<int,float>();
    // reg->addComponent<bool>(v1);
    // reg->removeComponent<bool>(v1);
    reg->create<float,int>();
    reg->iterate<int&>(f1);
    reg->iterate<int>([](std::array<void*,2> arg,size_t chunk_size){
        printf("{");
        for (size_t i = 0; i < chunk_size; i++)
        {
            printf("%d, ",((int*)arg[0])[i]);
        }
        printf("}\n");
    });
    reg->iterate<int>(f2);
    

   reg->addSystem<TransformSystem>();

   reg->executeSystems();*/


    delete reg;

    DOTS::ThreadPool tp(4);
    tp.wait();
    tp.addJob(DOTS::Job{ 
        [](DOTS::entity_t e){printf("next!\n"); if(e<10)e++; return e;}, 
        [](DOTS::entity_t e1,DOTS::entity_t e2){printf("proc(%u,%u) %llu\n",e1,e2,std::this_thread::get_id());}
    },0);
    tp.restart();
    tp.wait();

    printf("done\n");
}