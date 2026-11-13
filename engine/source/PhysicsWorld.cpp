#include "Physics/PhysicsWorld.h"
#include "Physics/PhysicsBody.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>

#include <raylib.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <thread>


namespace ScrapperEngine {
namespace {
constexpr JPH::uint MaxBodies = 4096;
constexpr JPH::uint MaxBodyPairs = 4096;
constexpr JPH::uint MaxContactConstraints = 4096;
constexpr JPH::uint BroadPhaseLayerCount = 2;
constexpr std::size_t TempAllocatorBytes = 8 * 1024 * 1024;

void TraceToLog(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    TraceLog(LOG_INFO, "JOLT: %s", buffer);
}
}

PhysicsWorld::PhysicsWorld() = default;
PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::Initialize() {
    if (initialized) {
        return;
    }

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceToLog;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    layerPairFilter = std::make_unique<JPH::ObjectLayerPairFilterTable>(PhysicsLayers::Count);
    layerPairFilter->EnableCollision(PhysicsLayers::Moving, PhysicsLayers::Static);
    layerPairFilter->EnableCollision(PhysicsLayers::Moving, PhysicsLayers::Moving);

    broadPhaseLayers = std::make_unique<JPH::BroadPhaseLayerInterfaceTable>(PhysicsLayers::Count, BroadPhaseLayerCount);
    broadPhaseLayers->MapObjectToBroadPhaseLayer(PhysicsLayers::Static, JPH::BroadPhaseLayer(0));
    broadPhaseLayers->MapObjectToBroadPhaseLayer(PhysicsLayers::Moving, JPH::BroadPhaseLayer(1));

    broadPhaseFilter = std::make_unique<JPH::ObjectVsBroadPhaseLayerFilterTable>(
        *broadPhaseLayers, BroadPhaseLayerCount, *layerPairFilter, PhysicsLayers::Count);

    system = std::make_unique<JPH::PhysicsSystem>();
    system->Init(MaxBodies, 0, MaxBodyPairs, MaxContactConstraints,
                 *broadPhaseLayers, *broadPhaseFilter, *layerPairFilter);

    tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(TempAllocatorBytes);
    const int workerThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workerThreads);

    accumulator = 0.0f;
    initialized = true;
}

void PhysicsWorld::Shutdown() {
    if (!initialized) {
        return;
    }

    bodies.clear();
    jobSystem.reset();
    tempAllocator.reset();
    system.reset();
    broadPhaseFilter.reset();
    broadPhaseLayers.reset();
    layerPairFilter.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    initialized = false;
}

void PhysicsWorld::Step(float deltaTime) {
    if (!initialized) {
        return;
    }

    // Clamp so a long hitch can't snowball into ever more catch-up steps.
    accumulator = std::min(accumulator + deltaTime, FixedDeltaTime * MaxStepsPerFrame);

    bool stepped = false;
    while (accumulator >= FixedDeltaTime) {
        for (PhysicsBody* body : bodies) {
            body->PrePhysicsStep(FixedDeltaTime);
        }

        system->Update(FixedDeltaTime, 1, tempAllocator.get(), jobSystem.get());

        for (PhysicsBody* body : bodies) {
            body->PostPhysicsStep();
        }

        accumulator -= FixedDeltaTime;
        stepped = true;
    }

    // Transforms only need the final pose of the frame.
    if (stepped) {
        for (PhysicsBody* body : bodies) {
            body->PullDynamicPose();
        }
    }
}

JPH::BodyInterface& PhysicsWorld::GetBodyInterface() {
    return system->GetBodyInterface();
}

void PhysicsWorld::Register(PhysicsBody* body) {
    bodies.push_back(body);
}

void PhysicsWorld::Unregister(PhysicsBody* body) {
    auto it = std::find(bodies.begin(), bodies.end(), body);
    if (it != bodies.end()) {
        *it = bodies.back();
        bodies.pop_back();
    }
}

}
