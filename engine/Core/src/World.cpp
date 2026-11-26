#include "Scrapper/Core/World.h"
#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Transform.h"
#include "HierarchyState.h"

#include <iterator>
#include <vector>


namespace Scrapper {
namespace {
void MarkOrderDirty(entt::registry& registry, entt::entity) {
    registry.ctx().get<HierarchyState>().orderDirty = true;
}

void MarkActivityDirty(entt::registry& registry, entt::entity) {
    registry.ctx().get<HierarchyState>().activityDirty = true;
}

void AttachWorldTransform(entt::registry& registry, entt::entity entity) {
    registry.emplace_or_replace<WorldTransform>(entity, Transforms::ComputeWorld(registry, entity));
    MarkOrderDirty(registry, entity);
}

void DetachWorldTransform(entt::registry& registry, entt::entity entity) {
    registry.remove<WorldTransform>(entity);
    MarkOrderDirty(registry, entity);
}

void DestroyPending(World& world, float) {
    entt::registry& registry = world.Registry();
    auto view = registry.view<PendingDestroy>();
    const std::vector<entt::entity> doomed(view.begin(), view.end());
    registry.destroy(doomed.begin(), doomed.end());
}
}

World::World() {
    registry.ctx().emplace<HierarchyState>();

    registry.on_construct<LocalTransform>().connect<&AttachWorldTransform>();
    registry.on_destroy<LocalTransform>().connect<&DetachWorldTransform>();
    registry.on_construct<Parent>().connect<&MarkOrderDirty>();
    registry.on_update<Parent>().connect<&MarkOrderDirty>();
    registry.on_destroy<Parent>().connect<&MarkOrderDirty>();
    registry.on_construct<Inactive>().connect<&MarkActivityDirty>();
    registry.on_destroy<Inactive>().connect<&MarkActivityDirty>();

    // Phase anchors: empty nodes chained in order, for modules to place systems between.
    const char* phases[] = { Phase::Input, Phase::Update, Phase::PreSimulate, Phase::Simulate, Phase::PostSimulate,
                             Phase::LateUpdate, Phase::Render, Phase::Cleanup, Phase::End };
    for (std::size_t i = 0; i < std::size(phases); ++i) {
        Placement placement;
        if (i > 0) placement.After(phases[i - 1]);
        pipeline.Add(phases[i], {}, placement);
    }

    const auto propagate = [](World& w, float) { PropagateTransforms(w.Registry()); };
    pipeline.Add(CoreSystem::PropagatePreSimulate, propagate, Placement::InPhase(Phase::PreSimulate));
    pipeline.Add(CoreSystem::PropagatePostSimulate, propagate, Placement::InPhase(Phase::PostSimulate));
    pipeline.Add(CoreSystem::PropagateRender, propagate, Placement::InPhase(Phase::Render));
    pipeline.Add(CoreSystem::Cleanup, &DestroyPending, Placement::InPhase(Phase::Cleanup));
}

World::~World() {
    registry.clear();
}

}
