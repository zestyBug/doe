#if !defined(TYPEID_HPP)
#define TYPEID_HPP
#include <algorithm>    // std::sort
#include "cutil/static_array.hpp"
#include "cutil/span.hpp"
#include "cutil/basics.hpp"
#include "ECS/Base/Entity.hpp"
namespace ECS {
    struct ISharedComponentData {};
    struct IComponentData {};
    struct IManagedComponentData {
        virtual ~IManagedComponentData() {}
    };
    struct TypeID final {
        TypeID()=default;
        TypeID(uint32_t v):value{v}{};
        TypeID(const TypeID&)=default;
        TypeID& operator = (const TypeID&)=default;
        TypeID(TypeID&&)=default;
        TypeID& operator = (TypeID&&)=default;
        bool operator == (const TypeID& v) const { return this->value == v.value; }
        bool operator != (const TypeID& v) const { return this->value != v.value; }
        bool operator <  (const TypeID& v) const { return this->value <  v.value; }
        bool operator >  (const TypeID& v) const { return this->value <  v.value; }
        inline uint32_t index() const;
        inline uint32_t flags() const;
        inline bool isSharedComponent() const;
        inline bool isZeroSized() const;
        inline bool isManaged() const ;
        /// @brief MAGIC NUMBER, Maximum number of unique component types supported by the TypeManager
        /// @details Considerations: SharedComponentIndex can only use up to 19 bit for the sharead component type
        /// TypeManager::ClearFlagsMask this number must be power of 2 because of bit
        static constexpr uint16_t MaximumTypesCount = 1 << 11;
        static bool compare(const_span<TypeID> v1,const_span<TypeID> v2)
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
        uint32_t value=0;
    };

    class TypeManager
    {
    public:
        /// @brief MAGIC NUMBER
        /// @details Considerations: SharedComponent must comes before ZeroSized components
        static constexpr uint32_t SharedComponentTypeFlag  = 1 << 29;
        /// @brief MAGIC NUMBER, destruction is managed by the engine
        /// @details Considerations: SharedComponent must comes before ZeroSized components
        static constexpr uint32_t ManagedComponentTypeFlag = 1 << 17;
        static constexpr uint32_t ZeroSizeInChunkTypeFlag  = 1 << 28;
        static constexpr uint32_t ClearFlagsMask = TypeID::MaximumTypesCount-1;
        enum class TypeCategory : uint16_t {
            /// Implements IComponentData (can be either a struct or a class)
            IComponentData,
            /// Implements ISharedComponentData (can be either a struct or a class)
            ISharedComponentData,
            /// Is an Entity
            Entity,
        };
        typedef void(*DefaultFunction)(void*);
        struct TypeInfo {
            TypeID       TypeIndex;
            /// @brief Blittable size of the component type.
            uint16_t     TypeSize = 0;
            /// @brief The number of bytes used in a Chunk to store an instance of this component.
            /// @note this includes internal capacity and header overhead for buffers. Also, note
            /// that components with no member variables will have a SizeInChunk of 0, but will have a
            /// TypeSize of GREATER than 0 (since C++ does not allow for zero-sized types).
            uint16_t     SizeInChunk = 0;
            /// @brief The alignment requirement for the component.
            uint16_t     AlignmentInBytes;
            TypeCategory Category;
            DefaultFunction defaultConstruct;
            DefaultFunction defaultDestruct;
            /// @brief Returns true if the component does not require space in Chunk memory
            bool         IsZeroSized() {return SizeInChunk==0;}
        };
    private:
        static uint32_t    typeCount;
        static TypeInfo    sharedTypeInfos[TypeID::MaximumTypesCount];
        static const char *sharedTypeNames[TypeID::MaximumTypesCount];
    public:
        static inline const_span<TypeInfo> GetTypeInfoPointer(){
            return {sharedTypeInfos, typeCount};
        }
        static uint32_t GetTypeCount() {
            return typeCount;
        }
        static const TypeInfo GetTypeInfo(TypeID typeIndex) {
            if(typeIndex.index() >= typeCount) throw std::bad_typeid();
            return sharedTypeInfos[typeIndex.index()];
        }
        static const char* GetTypeName(TypeID typeIndex) {
            if(typeIndex.index() >= typeCount) throw std::bad_typeid();
            return sharedTypeNames[typeIndex.index()];
        }
        static TypeID registerNull();
        static TypeID registerEntity();
        template<typename T>
        static TypeID registerType(const char* name) {
            // MAGIC NUMBER
            static_assert(sizeof(T) <= 0x800 && alignof(T) <= 0x1000);
            static_assert(
                std::is_class_v<T> && std::is_default_constructible_v<T> && (
                std::is_same_v<T,Entity> || 
                std::is_base_of_v<IComponentData,T> || 
                (std::is_base_of_v<ISharedComponentData,T> && !std::is_empty_v<T>)
                )
            );
            static_assert(!(std::is_base_of_v<IComponentData,T> && std::is_base_of_v<ISharedComponentData,T>));
            if(unlikely(typeCount >= TypeID::MaximumTypesCount))
                throw std::runtime_error("TypeID::MaximumTypesCount");

            uint32_t index = typeCount++;

            uint32_t value = index;
            if(std::is_base_of_v<ISharedComponentData,T>)
                value |= SharedComponentTypeFlag;
            if(std::is_base_of_v<IManagedComponentData,T>)
                value |= ManagedComponentTypeFlag;
            if(std::is_empty_v<T>)
                value |= ZeroSizeInChunkTypeFlag;
            sharedTypeInfos[index].TypeIndex = TypeID(value);

            sharedTypeInfos[index].TypeSize = sizeof(T);
            sharedTypeInfos[index].SizeInChunk = std::is_empty_v<T> ? 0 : sizeof(T);
            sharedTypeInfos[index].AlignmentInBytes = alignof(T);

            if(std::is_same_v<T,Entity>)
                sharedTypeInfos[index].Category = TypeCategory::Entity;
            else if(std::is_base_of_v<IComponentData,T>)
                sharedTypeInfos[index].Category = TypeCategory::IComponentData;
            else if(std::is_base_of_v<ISharedComponentData,T>)
                sharedTypeInfos[index].Category = TypeCategory::ISharedComponentData;
            else
                throw std::bad_typeid();
            sharedTypeInfos[index].defaultDestruct  = [](void* x){static_cast<T*>(x)->~T();};
            sharedTypeInfos[index].defaultConstruct = [](void* x){new (static_cast<T*>(x)) T();};
            sharedTypeNames[index]=name;
            return sharedTypeInfos[index].TypeIndex;
        }
    };

    template<typename> TypeID __typeid__() = delete;
    template<> TypeID __typeid__<nullptr_t>();
    template<> TypeID __typeid__<ECS::Entity>();

    uint32_t TypeID::index() const {return this->value & TypeManager::ClearFlagsMask;}
    uint32_t TypeID::flags() const {return this->value & ~TypeManager::ClearFlagsMask;}
    bool TypeID::isSharedComponent() const {return this->value & TypeManager::SharedComponentTypeFlag;}
    bool TypeID::isZeroSized() const {return this->value & TypeManager::ZeroSizeInChunkTypeFlag;}
    bool TypeID::isManaged() const {return this->value & TypeManager::ManagedComponentTypeFlag;}

    template<typename T>
    inline TypeID getTypeID() { return __typeid__<std::remove_const_t<std::remove_reference_t<T>>>(); }

    namespace internal
    {
        inline bool customCompare (TypeID a,TypeID b) {
            if(unlikely(a == b))
                throw std::bad_typeid();
            return a < b;
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
    const_span<uint16_t> componentIndecies() {
        static const static_array<uint16_t,sizeof...(T)> ret = internal::_INIT_COMPONENTS_INDECIES_<T...>();
        return {ret.data(),ret.size()};
    }
    template<typename ... T>
    const_span<TypeID> componentTypes() {
        static const static_array<TypeID,sizeof...(T)> ret = internal::_INIT_COMPONENTS_TYPES_<T...>();
        return {ret.data(),ret.size()};
    }

}

#endif
