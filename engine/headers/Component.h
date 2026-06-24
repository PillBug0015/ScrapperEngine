#pragma once


namespace ScrapperEngine {
class GameObject;

class Component {
private:
    bool enabled = true;
    bool started = false;
    bool destroyed = false;

public:
    GameObject* owner = nullptr;

    virtual ~Component() = default;

    virtual void Start() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void Update(float /*deltaTime*/) {}

    bool IsEnabled() const {
        return enabled && !destroyed;
    }

    bool IsDestroyed() const {
        return destroyed;
    }

    void SetEnabled(bool newEnabled) {
        if (destroyed || enabled == newEnabled) {
            return;
        }

        enabled = newEnabled;

        if (enabled && started) {
            OnEnable();
        } else if (started) {
            OnDisable();
        }
    }

    void Destroy() {
        DestroyLifecycle(true);
    }

private:
    friend class GameObject;

    bool IsRawEnabled() const {
        return enabled;
    }

    bool HasStarted() const {
        return started;
    }

    void DestroyLifecycle(bool ownerActiveInHierarchy) {
        if (destroyed) {
            return;
        }

        if (ownerActiveInHierarchy && enabled && started) {
            OnDisable();
        }

        destroyed = true;
        OnDestroy();
        owner = nullptr;
    }

    void StartLifecycle() {
        if (destroyed || started) {
            return;
        }

        started = true;
        Start();

        if (enabled && !destroyed) {
            OnEnable();
        }
    }
};

}
