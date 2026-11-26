#pragma once

#include "Scrapper/Core/Pipeline.h"

#include <entt/entity/registry.hpp>

#include <utility>


namespace Scrapper {
namespace CoreSystem {
inline constexpr const char* PropagatePreSimulate = "Core.Propagate.PreSimulate";
inline constexpr const char* PropagatePostSimulate = "Core.Propagate.PostSimulate";
inline constexpr const char* PropagateRender = "Core.Propagate.Render";
inline constexpr const char* Cleanup = "Core.Cleanup";
}

// Data (EnTT registry: plain component arrays + resources) and the pipeline that runs over it.
class World {
public:
    World();
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    entt::registry& Registry() { return registry; }
    const entt::registry& Registry() const { return registry; }
    Pipeline& GetPipeline() { return pipeline; }

    // Module-wide singletons (physics world, renderer state, network session...).
    template <typename T, typename... Args>
    T& AddResource(Args&&... args) { return registry.ctx().emplace<T>(std::forward<Args>(args)...); }
    template <typename T>
    T& Resource() { return registry.ctx().get<T>(); }
    template <typename T>
    T* TryResource() { return registry.ctx().find<T>(); }
    template <typename T>
    void RemoveResource() { registry.ctx().erase<T>(); }

    void Step(float deltaTime) { pipeline.Run(*this, deltaTime); }

    void RequestQuit() { quitRequested = true; }
    bool IsQuitRequested() const { return quitRequested; }

private:
    entt::registry registry;
    Pipeline pipeline;
    bool quitRequested = false;
};

}
