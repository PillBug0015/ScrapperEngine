#pragma once

#include "Scrapper/Jolt/JoltRuntime.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <memory>

namespace JPH {
class PhysicsSystem;
class BodyInterface;
class TempAllocatorImpl;
class JobSystemThreadPool;
class ObjectLayerPairFilterTable;
class BroadPhaseLayerInterfaceTable;
class ObjectVsBroadPhaseLayerFilterTable;
}


namespace Scrapper {
namespace PhysicsLayers {
inline constexpr JPH::ObjectLayer Static = 0;
inline constexpr JPH::ObjectLayer Moving = 1;
inline constexpr JPH::uint Count = 2;
}

// World resource: the Jolt physics system. Reach for it directly for constraints and queries.
class JoltWorld {
public:
    static constexpr float FixedDeltaTime = 1.0f / 60.0f;
    static constexpr int MaxStepsPerFrame = 4;

    JoltWorld();
    ~JoltWorld();
    JoltWorld(const JoltWorld&) = delete;
    JoltWorld& operator=(const JoltWorld&) = delete;

    JPH::PhysicsSystem& System() { return *system; }
    JPH::BodyInterface& Bodies();
    void Update(float deltaTime); // one fixed step

    float accumulator = 0.0f;

private:
    JoltRuntime runtime; // first: torn down last
    std::unique_ptr<JPH::ObjectLayerPairFilterTable> layerPairFilter;
    std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> broadPhaseLayers;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> broadPhaseFilter;
    std::unique_ptr<JPH::PhysicsSystem> system;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
};

}
