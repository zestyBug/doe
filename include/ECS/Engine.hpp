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
        EntityComponentStore ecs;
        EntityQueryManager eqm;
        /// @brief List of systems (Intenal)
        std::vector<std::unique_ptr<ISystem>> sys;
        /// Intenal
        AssetsManager am;
        ResourceManager rm;
        DOE(){
            sys.reserve(Constants::InitialSystemCapacity);
        };
    };
    /// @brief A global singleton for the engine.
    /// @note Must be accessed only by the thread pool and the main function.
    extern std::unique_ptr<DOE> sharedEngine;
} // namespace ECS


#endif // ENGINE_HPP
