#include "Scrapper/Jolt/JoltWorld.h"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>

#include <algorithm>
#include <thread>


namespace Scrapper {
namespace {
constexpr JPH::uint MaxBodies = 4096;
constexpr JPH::uint MaxBodyPairs = 4096;
constexpr JPH::uint MaxContactConstraints = 4096;
constexpr JPH::uint BroadPhaseLayerCount = 2;
constexpr std::size_t TempAllocatorBytes = 8 * 1024 * 1024;
}

JoltWorld::JoltWorld() {
    layerPairFilter = std::make_unique<JPH::ObjectLayerPairFilterTable>(PhysicsLayers::Count);
    layerPairFilter->EnableCollision(PhysicsLayers::Moving, PhysicsLayers::Static);
    layerPairFilter->EnableCollision(PhysicsLayers::Moving, PhysicsLayers::Moving);

    broadPhaseLayers = std::make_unique<JPH::BroadPhaseLayerInterfaceTable>(PhysicsLayers::Count, BroadPhaseLayerCount);
    broadPhaseLayers->MapObjectToBroadPhaseLayer(PhysicsLayers::Static, JPH::BroadPhaseLayer(0));
    broadPhaseLayers->MapObjectToBroadPhaseLayer(PhysicsLayers::Moving, JPH::BroadPhaseLayer(1));

    broadPhaseFilter = std::make_unique<JPH::ObjectVsBroadPhaseLayerFilterTable>(
        *broadPhaseLayers, BroadPhaseLayerCount, *layerPairFilter, PhysicsLayers::Count);

    system = std::make_unique<JPH::PhysicsSystem>();
    system->Init(MaxBodies, 0, MaxBodyPairs, MaxContactConstraints, *broadPhaseLayers, *broadPhaseFilter, *layerPairFilter);

    tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(TempAllocatorBytes);
    const int workerThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workerThreads);
}

JoltWorld::~JoltWorld() = default; // members go in reverse: job system first, runtime last

JPH::BodyInterface& JoltWorld::Bodies() {
    return system->GetBodyInterface();
}

void JoltWorld::Update(float deltaTime) {
    system->Update(deltaTime, 1, tempAllocator.get(), jobSystem.get());
}

}
