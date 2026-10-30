#if !defined(TYPEID_HPP)
#define TYPEID_HPP
#include <algorithm>    // std::sort
#include "cutil/static_array.hpp"
#include "cutil/span.hpp"
#include "cutil/string_view.hpp"
#include "cutil/basics.hpp"
#include "ECS/Base/Entity.hpp"
#include "ECS/Base/Resource.hpp"
#include "Constants.hpp"
namespace ECS {
    struct EntityQueryData;
    class TypeManager;
    struct Archetype;
    struct EntityComponentStore;
    struct ComponentDependencyManager;

    struct ISharedComponentData {};
    struct IComponentData {};
    struct ResourceID final {
        ResourceID(const ResourceID&)=default;
        ResourceID& operator = (const ResourceID&)=default;
        ResourceID(ResourceID&&)=default;
        ResourceID& operator = (ResourceID&&)=default;
        inline uint32_t value() const { return this->_value; }
        ResourceID()=default;
    private:
        friend class TypeManager;
        inline static ResourceID fromIndex(uint32_t v){
            ResourceID r;r._value = v;return r;
        };
        uint32_t _value=0;
    };
    struct ResourceEntry;
    struct IResource {
        void load(void *ctx, Resource resource){};
        void free(void *ctx, Resource resource){};
    };
    /// @warning TypeIDs are expected to have flags included with them. undefined behaviour otherwise.
    struct TypeID final {
        /// @brief Just in case you know what you are doing
        /// @details if you forget to include flags, the engine may behave unexpectedly.
        inline static TypeID fromIndex(uint32_t v){
            TypeID r;r._value = v;return r;
        };
        TypeID(const TypeID&)=default;
        TypeID& operator = (const TypeID&)=default;
        TypeID(TypeID&&)=default;
        TypeID& operator = (TypeID&&)=default;
        /// @brief Flags are included.
        bool operator == (const TypeID& v) const { return this->_value == v._value; }
        /// @brief Flags are included.
        bool operator != (const TypeID& v) const { return this->_value != v._value; }
        /* 
         * There is a different between value and index.
         * so "operator <" and "operator >"  are not defined. 
         */
        inline uint32_t value() const;
        inline uint32_t index() const;
        inline uint32_t flags() const;
        inline bool isSharedComponent() const;
        inline bool isZeroSized() const;
        inline bool hasAssetRef() const;
        static bool compare(span<const TypeID> v1,span<const TypeID> v2)
        {
            if (v1.size() != v2.size())
                return false;
            for (uint32_t i = 0; i < v1.size(); ++i)
                if (v1[i] != v1[i])
                    return false;
            return true;
        }
        void Debug();
    private:
        friend struct EntityQueryBuilder;
        friend struct EntityQueryData;
        friend class TypeManager;
        friend struct Archetype;
        friend struct EntityComponentStore;
        friend struct ComponentDependencyManager;
        TypeID()=default;
        uint32_t _value=0;
    };

    class TypeManager
    {
    public:
        /// @brief MAGIC NUMBER
        /// @details Considerations: SharedComponent must comes before ZeroSized components
        static constexpr uint32_t SharedComponentTypeFlag  = 1 << 29;
        static constexpr uint32_t ZeroSizeInChunkTypeFlag  = 1 << 28;
        static constexpr uint32_t AssetRefFlag             = 1 << 18;
        static constexpr uint32_t ClearFlagsMask = Constants::MaximumTypesCount-1;
        enum class TypeCategory : uint16_t {
            /// Implements IComponentData (can be either a struct or a class)
            IComponentData = 0,
            /// Implements ISharedComponentData (can be either a struct or a class)
            ISharedComponentData,
            /// Is an Entity
            Entity,
        };
        typedef void(DefaultFunction)(void*);
        typedef void(ResourceCBFunction)(void*,void*,Resource);
        struct ResourceInfo {
            void          *ctx = nullptr;
            ResourceCBFunction *load = nullptr;
            ResourceCBFunction *free = nullptr;
            uint32_t       typeSize = 0;
            const char    *typeName = nullptr;
            ResourceID type;
        };
        struct TypeInfo {
            TypeID       TypeIndex;
            /// @brief Blittable size of the component type.
            uint16_t     TypeSize = 0;
            /// @brief The number of bytes used in a Chunk to store an instance of this component.
            /// @note Components with no member variables will have a SizeInChunk of 0, but will have a
            /// TypeSize of GREATER than 0 (since C++ does not allow for zero-sized types).
            uint16_t     SizeInChunk = 0;
            /// @brief The alignment requirement for the component.
            uint16_t     AlignmentInBytes = 0;
            TypeCategory Category = TypeCategory::IComponentData;
            uint32_t     AssetRefOffsetStartIndex = 0;
            uint32_t     AssetRefOffsetCount = 0;
            DefaultFunction *defaultConstruct = nullptr;
            DefaultFunction *defaultDestruct = nullptr;
            const char  *Name = nullptr;
            /// @brief Returns true if the component does not require space in Chunk memory
            bool         IsZeroSized() {return SizeInChunk==0;}
        };
    private:
        static uint32_t    initialized;
        static uint32_t    typeCount;
        static uint32_t    resourceTypeCount;
        static uint32_t    assetRefOffsetIndex;
        static std::unique_ptr<TypeInfo[]> sharedTypeInfos;
        static std::unique_ptr<uint16_t[]> assetRefOffsetList;
        static std::unique_ptr<ResourceInfo[]> sharedResourceInfo;
    public:
        static void Initialize();
        static void Dispose();
        static const ResourceInfo& GetResourceInfo(ResourceID rType) {
            if(rType.value() >= resourceTypeCount) throw std::bad_typeid();
            return sharedResourceInfo[rType.value()];
        }
        /// @brief Register informations about a resource globally
        /// @param ctx A pointer to the context for the manager load and free functions
        /// @tparam T Base of IResource
        /// @return A unique id
        template<typename T>
        static ResourceID registerResource(void* ctx) {
            if(unlikely(resourceTypeCount >= Constants::MaximumResourceTypeCount))
                throw std::runtime_error("registerResourceType(): Constants::MaximumResourceTypeCount");
            static_assert(!std::is_empty_v<T> && sizeof(T) < 256 && std::is_base_of_v<IResource,T>);
            const uint32_t index = resourceTypeCount++;
            ResourceInfo &info = sharedResourceInfo[index];
            info.load = [](void *thiz,void *ctx,Resource r){static_cast<T*>(thiz)->load(ctx,r);};
            info.free = [](void *thiz,void *ctx,Resource r){static_cast<T*>(thiz)->free(ctx,r);};
            info.ctx = ctx;
            info.typeSize = sizeof(T);
            info.typeName = TypeManager::GetTypeName<T>();
            info.type = ResourceID::fromIndex(index);
            return ResourceID::fromIndex(index);
        }
        static inline span<const uint16_t> GetAssetRefOffsetsPointer(){
            return {assetRefOffsetList.get(), assetRefOffsetIndex};
        }
        static inline span<const TypeInfo> GetTypeInfoPointer(){
            return {sharedTypeInfos.get(), typeCount};
        }
        static uint32_t GetTypeCount() {
            return typeCount;
        }
        static const TypeInfo& GetTypeInfo(TypeID typeIndex) {
            if(typeIndex.index() >= typeCount) throw std::bad_typeid();
            return sharedTypeInfos[typeIndex.index()];
        }
        static const TypeInfo& GetTypeInfo(uint32_t index) {
            if(index >= typeCount) throw std::bad_typeid();
            return sharedTypeInfos[index];
        }
        static const char* GetTypeName(TypeID typeIndex) {
            if(typeIndex.index() >= typeCount) throw std::bad_typeid();
            return sharedTypeInfos[typeIndex.index()].Name;
        }
        static const char* GetTypeName(uint32_t index) {
            if(index >= typeCount) throw std::bad_typeid();
            return sharedTypeInfos[index].Name;
        }
        template<class T>
        static constexpr const char* GetTypeName()
        {
        #if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
        #elif defined(_MSC_VER)
            return __FUNCSIG__;
        #else
            #error
        #endif
        }
        /// @brief Register informations about a component type.
        /// @note A component MUST be publicly default constuctable. And publicly copiable and destrcutable
        /// @warning Call this function only once per type.
        /// @tparam T Component
        /// @param assetRefs list of byte indecies that contain a pointer to a resouce, to handle garbage collection automated.
        /// @return An unique id that the informations are linked with.
        template<typename T>
        static TypeID registerType(span<const uint16_t> assetRefs = {}) {
            Initialize();
            // MAGIC NUMBER
            static_assert(sizeof(T) <= Constants::MaximumComponentSize && alignof(T) <= Constants::MaximumComponentSize);
            static_assert(std::is_class_v<T> && std::is_default_constructible_v<T>);
            static_assert(
                std::is_same_v<T,Entity> || 
                (std::is_base_of_v<IComponentData,T> && std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>) || 
                (std::is_base_of_v<ISharedComponentData,T> && !std::is_empty_v<T>)
            );
            if(unlikely(typeCount >= Constants::MaximumTypesCount))
                throw std::runtime_error("registerType(): Constants::MaximumTypesCount");

            const uint32_t index = typeCount++;

            uint32_t value = index;
            if constexpr (std::is_base_of_v<ISharedComponentData,T>)
                value |= SharedComponentTypeFlag;
            if constexpr (std::is_empty_v<T>)
                value |= ZeroSizeInChunkTypeFlag;
            if(!assetRefs.empty())
                value |= AssetRefFlag;
            TypeInfo &info = sharedTypeInfos[index];
            info.TypeIndex = TypeID::fromIndex(value);

            info.TypeSize = sizeof(T);
            info.SizeInChunk = std::is_empty_v<T> ? 0 : sizeof(T);
            info.AlignmentInBytes = alignof(T);

            if constexpr (std::is_same_v<T,Entity>)
                info.Category = TypeCategory::Entity;
            else if constexpr (std::is_base_of_v<IComponentData,T>)
                info.Category = TypeCategory::IComponentData;
            else if constexpr (std::is_base_of_v<ISharedComponentData,T>)
                info.Category = TypeCategory::ISharedComponentData;
            else
                throw std::bad_typeid();
            
            info.AssetRefOffsetStartIndex = assetRefOffsetIndex;
            info.AssetRefOffsetCount = assetRefs.size();
            if(!assetRefs.empty()){
                uint16_t *begin_dst = assetRefOffsetList.get() + assetRefOffsetIndex;
                const uint16_t *begin_src = assetRefs.data();
                const uint16_t *end_src   = assetRefs.data() + assetRefs.size();
                assetRefOffsetIndex += assetRefs.size();
                if(Constants::MaximumRefOffsetCount < assetRefOffsetIndex)
                    throw std::runtime_error("registerType(): Constants::MaximumRefOffsetCount");
                while(begin_src < end_src) {
                    if(*begin_src >= sizeof(T))
                        throw std::runtime_error("registerType(): invalid AssetRefOffset");
                    *begin_dst = *begin_src;
                    begin_src++;
                    begin_dst++;
                }
            }

            info.defaultDestruct  = [](void* x){static_cast<T*>(x)->~T();};
            info.defaultConstruct = [](void* x){new (static_cast<T*>(x)) T();};
            info.Name = GetTypeName<T>();
            return info.TypeIndex;
        }
    };

    /// @brief A generic funcion you must define for each component type.
    /// @note use DEF_TYPE helper macro to define your __typeid__
    /// @return the id that is returned from registerType.
    template<typename> TypeID __typeid__() = delete;
    template<> TypeID __typeid__<std::nullptr_t>();
    template<> TypeID __typeid__<ECS::Entity>();
    #define DEF_TYPE(TYPE) template<> ECS::TypeID ECS::__typeid__<TYPE>(){static ECS::TypeID v = ECS::TypeManager::registerType<TYPE>();return v;}

    uint32_t TypeID::value() const {return this->_value;}
    uint32_t TypeID::index() const {return this->_value & TypeManager::ClearFlagsMask;}
    uint32_t TypeID::flags() const {return this->_value & ~TypeManager::ClearFlagsMask;}
    bool TypeID::isSharedComponent() const {return this->_value & TypeManager::SharedComponentTypeFlag;}
    bool TypeID::isZeroSized() const {return this->_value & TypeManager::ZeroSizeInChunkTypeFlag;}
    bool TypeID::hasAssetRef() const {return this->_value & TypeManager::AssetRefFlag;}

    /// @brief A compile-time helper to call __typeid__
    /// @warning Dont define getTypeID manually.
    template<typename T>
    inline TypeID getTypeID() { return __typeid__<std::remove_const_t<std::remove_reference_t<T>>>(); }
    template<typename T>
    constexpr bool isSharedComponent() { return std::is_base_of_v<ISharedComponentData,T> && !std::is_empty_v<T>; }

    template<typename> ResourceID __resource__() = delete;
    #define DEF_RESOURCE(TYPE,CTX) template<> ECS::ResourceID ECS::__resource__<TYPE>(){static ECS::ResourceID v = ECS::TypeManager::registerResource<TYPE>(CTX);return v;}
    template<typename T>
    inline ResourceID getResourceID() { return __typeid__<std::remove_const_t<std::remove_reference_t<T>>>(); }

    namespace internal
    {
        inline bool customCompare (TypeID a,TypeID b) {
            if(unlikely(a == b))
                throw std::bad_typeid();
            return a.value() < b.value();
        }
        template<typename ... T>
        static_array<uint16_t,sizeof...(T)> _INIT_COMPONENTS_INDECIES_() {
            static_array<uint16_t,sizeof...(T)> ret;
            (ret.emplace_back(getTypeID<T>()), ...);
            std::sort(ret.begin(),ret.end(),customCompare);
            return ret;
        }
        template<typename ... T>
        static_array<TypeID,sizeof...(T)> _INIT_COMPONENTS_TYPES_() {
            static_array<TypeID,sizeof...(T)> ret;
            (ret.emplace_back(getTypeID<T>()), ...);
            std::sort(ret.begin(),ret.end(),customCompare);
            return ret;
        }
    } // namespace internal

    template<typename ... T>
    span<const uint16_t> componentIndecies() {
        static const static_array<uint16_t,sizeof...(T)> ret = internal::_INIT_COMPONENTS_INDECIES_<T...>();
        return {ret.data(),ret.size()};
    }
    template<typename ... T>
    span<const TypeID> componentTypes() {
        static const static_array<TypeID,sizeof...(T)> ret = internal::_INIT_COMPONENTS_TYPES_<T...>();
        return {ret.data(),ret.size()};
    }

}

#endif
