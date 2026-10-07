#if !defined(ENGINE_HPP)
#define ENGINE_HPP

#include "AssetsManager.hpp"
#include "ResourceManager.hpp"
#include "EntityComponentStore.hpp"
#include "EntityQueryManager.hpp"
#include "Base/ISystem.hpp"

namespace ECS
{
    struct JobChunkWrapperBase;
    /// @brief This struct must remain passive, no external calls
    struct DOE final {
        /// @brief Entities & components are stored here
        EntityComponentStore ecs;
        /// @brief Entiry query are stored here
        EntityQueryManager eqm;
        /// @brief List of systems (Intenal)
        std::vector<std::unique_ptr<ISystem>> sys;
        /// @brief Manages loading resources from budles. An asset is a bundle of resources  (Intenal)
        AssetsManager am;
        /// @brief Manages initialization, refrence counting and destruction of multiple resource types
        ResourceManager rm;
        DOE(){
            sys.reserve(Constants::InitialSystemCapacity);
        };
    };
    extern std::unique_ptr<DOE> sharedEngine;
} // namespace ECS


#endif // ENGINE_HPP
