#pragma once

#include "Scrapper/ScriptEngine/GameObject.h"

#include <memory>
#include <type_traits>
#include <vector>


namespace Scrapper {
// Game logic attached to a GameObject. Scripts run on the main thread in their own pipeline
// steps, so they may read and write any data synchronously.
class Script {
public:
    virtual ~Script() = default;

    virtual void Start() {}               // right before the first Update
    virtual void Update(float /*deltaTime*/) {}     // Phase::Update
    virtual void LateUpdate(float /*deltaTime*/) {} // Phase::LateUpdate, after physics
    virtual void OnDestroy() {}

    bool enabled = true;
    GameObject gameObject;

protected:
    World& GetWorld() const { return gameObject.GetWorld(); }

private:
    friend struct ScriptSystems;
    bool started = false;
};

// Move-only on purpose: a vector of unique_ptr claims to be copyable, which would make EnTT try to copy it.
struct ScriptList {
    ScriptList() = default;
    ScriptList(ScriptList&&) = default;
    ScriptList& operator=(ScriptList&&) = default;
    ScriptList(const ScriptList&) = delete;
    ScriptList& operator=(const ScriptList&) = delete;

    std::vector<std::unique_ptr<Script>> scripts;
};

template <typename T, typename... Args>
T& GameObject::AddScript(Args&&... args) {
    static_assert(std::is_base_of<Script, T>::value, "T must derive from Script");
    auto script = std::make_unique<T>(std::forward<Args>(args)...);
    script->gameObject = *this;
    T& ref = *script;
    world->Registry().get_or_emplace<ScriptList>(entity).scripts.push_back(std::move(script));
    return ref;
}

template <typename T>
T* GameObject::GetScript() const {
    if (const ScriptList* list = TryGet<ScriptList>()) {
        for (const auto& script : list->scripts) {
            if (T* typed = dynamic_cast<T*>(script.get())) return typed;
        }
    }
    return nullptr;
}

}
