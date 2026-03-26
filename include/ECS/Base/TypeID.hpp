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
        // MAGIC NUMBER 
        static constexpr uint16_t MaxTypeCount = 0x2000;
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
        static constexpr uint32_t SharedComponentTypeFlag  = 1 << 29;
        static constexpr uint32_t ManagedComponentTypeFlag = 1 << 17;
        static constexpr uint32_t ZeroSizeInChunkTypeFlag  = 1 << 28;
        /// @brief MAGIC NUMBER, Maximum number of unique component types supported by the TypeManager
        static constexpr uint32_t MaximumTypesCount = 1 << 10;
        static constexpr uint32_t ClearFlagsMask = MaximumTypesCount-1;
        enum class TypeCategory : uint16_t {
            /// Implements IComponentData (can be either a struct or a class)
            IComponentData,
            /// Implements ISharedComponentData (can be either a struct or a class)
            ISharedComponentData,
            /// Is an Entity
            Entity,
        };
        struct TypeInfo {
            TypeID       TypeIndex;
            /// The alignment requirement for the component.
            /// @brief Blittable size of the component type.
            uint16_t     TypeSize = 0;
            uint16_t     AlignmentInBytes;
            TypeCategory Category;
            /// @brief Returns true if the component does not require space in Chunk memory
            bool         IsZeroSized() {return TypeSize==0;}
        };
    private:
        static uint32_t    typeCount;
        static TypeInfo    sharedTypeInfos[MaximumTypesCount];
        static const char *sharedTypeNames[MaximumTypesCount];
    public:
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
        static TypeID registerNull() {
            // MAGIC NUMBER
            sharedTypeInfos[0].TypeIndex = TypeID(ZeroSizeInChunkTypeFlag);
            sharedTypeInfos[0].AlignmentInBytes = alignof(nullptr_t);
            sharedTypeInfos[0].Category = TypeCategory::IComponentData;
            sharedTypeInfos[0].TypeSize = sizeof(nullptr_t);
            sharedTypeNames[0]="nullptr_t";
            return TypeID(ZeroSizeInChunkTypeFlag);
        }
        static TypeID registerEntity() {
            // MAGIC NUMBER
            static_assert(sizeof(Entity) == 8);
            sharedTypeInfos[1].TypeIndex = TypeID(1);
            sharedTypeInfos[1].AlignmentInBytes = alignof(Entity);
            sharedTypeInfos[1].Category = TypeCategory::Entity;
            sharedTypeInfos[1].TypeSize = sizeof(Entity);
            sharedTypeNames[1]="Entity";
            return TypeID(1);
        }
        template<typename T>
        static TypeID registerType(const char* name) {
            // MAGIC NUMBER
            static_assert(sizeof(T) <= 0x1000 && alignof(T) <= 0x1000);
            static_assert(std::is_same_v<T,Entity> || std::is_base_of_v<IComponentData,T> || (std::is_base_of_v<ISharedComponentData,T> && sizeof(T) > 0x0));
            static_assert(!(std::is_base_of_v<IComponentData,T> && std::is_base_of_v<ISharedComponentData,T>));
            if(unlikely(typeCount >= TypeID::MaxTypeCount))
                throw std::runtime_error("MaxTypeCount");

            uint32_t index = typeCount++;

            uint32_t value = index;
            if(std::is_base_of_v<ISharedComponentData,T>)
                value |= SharedComponentTypeFlag;
            if(std::is_base_of_v<IManagedComponentData,T>)
                value |= ManagedComponentTypeFlag;
            if(sizeof(T) < 1)
                value |= ZeroSizeInChunkTypeFlag;
            sharedTypeInfos[index].TypeIndex = TypeID(value);

            sharedTypeInfos[index].TypeSize = sizeof(T);
            sharedTypeInfos[index].AlignmentInBytes = alignof(T);

            if(std::is_same_v<T,Entity>)
                sharedTypeInfos[index].Category = TypeCategory::Entity;
            else if(std::is_base_of_v<IComponentData,T>)
                sharedTypeInfos[index].Category = TypeCategory::IComponentData;
            else if(std::is_base_of_v<ISharedComponentData,T>)
                sharedTypeInfos[index].Category = TypeCategory::ISharedComponentData;
            else
                throw std::bad_typeid();
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
