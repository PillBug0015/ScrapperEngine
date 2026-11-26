#include "Scrapper/ScriptEngine/ScriptEngineModule.h"
#include "Scrapper/ScriptEngine/Script.h"

#include <vector>


namespace Scrapper {
struct ScriptSystems {
    // Entities are listed up front and scripts are walked by index, re-fetching the list each time:
    // scripts may add scripts or objects while they run.
    template <typename Call>
    static void ForEachScript(entt::registry& registry, Call&& call) {
        auto view = registry.view<ScriptList>(entt::exclude<Disabled, PendingDestroy>);
        const std::vector<entt::entity> entities(view.begin(), view.end());

        for (entt::entity entity : entities) {
            for (std::size_t i = 0;; ++i) {
                ScriptList* list = registry.valid(entity) ? registry.try_get<ScriptList>(entity) : nullptr;
                if (list == nullptr || i >= list->scripts.size()) break;
                Script& script = *list->scripts[i];
                if (script.enabled) call(script);
            }
        }
    }

    static void Update(World& world, float deltaTime) {
        ForEachScript(world.Registry(), [deltaTime](Script& script) {
            if (!script.started) {
                script.started = true;
                script.Start();
            }
            script.Update(deltaTime);
        });
    }

    static void LateUpdate(World& world, float deltaTime) {
        ForEachScript(world.Registry(), [deltaTime](Script& script) {
            if (script.started) script.LateUpdate(deltaTime);
        });
    }

    static void Destroy(World& world, float) {
        for (auto [entity, list] : world.Registry().view<ScriptList, PendingDestroy>().each()) {
            for (auto& script : list.scripts) script->OnDestroy();
        }
    }
};

void ScriptEngineModule::Install(World& world) {
    Pipeline& pipeline = world.GetPipeline();
    pipeline.Add(ScriptSystem::Update, &ScriptSystems::Update, Placement::InPhase(Phase::Update));
    pipeline.Add(ScriptSystem::LateUpdate, &ScriptSystems::LateUpdate, Placement::InPhase(Phase::LateUpdate));
    pipeline.Add(ScriptSystem::Destroy, &ScriptSystems::Destroy, Placement::InPhase(Phase::Cleanup).Before(CoreSystem::Cleanup));
}

void ScriptEngineModule::Uninstall(World& world) {
    Pipeline& pipeline = world.GetPipeline();
    pipeline.Remove(ScriptSystem::Update);
    pipeline.Remove(ScriptSystem::LateUpdate);
    pipeline.Remove(ScriptSystem::Destroy);

    for (auto [entity, list] : world.Registry().view<ScriptList>().each()) {
        for (auto& script : list.scripts) script->OnDestroy();
    }
    world.Registry().clear<ScriptList>();
}

}
