#pragma once

#include "Component.h"
#include "RenderSystem.h"


namespace ScrapperEngine {
class CameraComponent : public Component {
public:
    int order = 0;

    void OnEnable() override {
        RenderSystem::Instance().RegisterCamera(this);
    }

    void OnDisable() override {
        RenderSystem::Instance().UnregisterCamera(this);
    }

    void OnDestroy() override {
        RenderSystem::Instance().UnregisterCamera(this);
    }

    virtual bool Begin() = 0;
    virtual void End() = 0;
};

}
