#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <memory>
#include <vector>

namespace JPH {
class PhysicsSystem;
class BodyInterface;
class TempAllocatorImpl;
class JobSystemThreadPool;
class ObjectLayerPairFilterTable;
class BroadPhaseLayerInterfaceTable;
class ObjectVsBroadPhaseLayerFilterTable;
}


namespace ScrapperEngine {
class PhysicsBody;

namespace PhysicsLayers {
inline constexpr JPH::ObjectLayer Static = 0;
inline constexpr JPH::ObjectLayer Moving = 1;
inline constexpr JPH::uint Count = 2;
}

// Owns the Jolt world and steps it at a fixed rate. PhysicsBody components register here to sync poses.
class PhysicsWorld {
public:
    static constexpr float FixedDeltaTime = 1.0f / 60.0f;
    static constexpr int MaxStepsPerFrame = 4;

    static PhysicsWorld& Instance() {
        static PhysicsWorld instance;
        return instance;
    }

    void Initialize();
    void Shutdown();
    void Step(float deltaTime);

    JPH::PhysicsSystem& GetSystem() { return *system; }
    JPH::BodyInterface& GetBodyInterface();

private:
    friend class PhysicsBody;
    void Register(PhysicsBody* body);
    void Unregister(PhysicsBody* body);

    PhysicsWorld();
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    std::unique_ptr<JPH::ObjectLayerPairFilterTable> layerPairFilter;
    std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> broadPhaseLayers;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> broadPhaseFilter;
    std::unique_ptr<JPH::PhysicsSystem> system;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;

    std::vector<PhysicsBody*> bodies;
    float accumulator = 0.0f;
    bool initialized = false;
};

}
