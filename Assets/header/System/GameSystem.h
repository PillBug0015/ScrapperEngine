#pragma once

#include "Component.h"

namespace MyMudGame {

enum class SystemState
{
    Active,
    Inactive,
    Paused
};

class GameSystem : public ScrapperEngine::Component
{
public:
    GameSystem()
        : currentState(SystemState::Inactive)
    {
        SetEnabled(false);
    }

    virtual ~GameSystem() = default;

    SystemState GetSystemState() const { return currentState; }
    bool IsSystemActive() const { return currentState == SystemState::Active; }
    bool IsSystemPaused() const { return currentState == SystemState::Paused; }

    void Activate()
    {
        if (currentState == SystemState::Active)
        {
            return;
        }

        currentState = SystemState::Active;
        SetEnabled(true);
        OnActivated();
    }

    void Deactivate()
    {
        if (currentState == SystemState::Inactive)
        {
            return;
        }

        currentState = SystemState::Inactive;
        OnDeactivated();
        SetEnabled(false);
    }

    void Pause()
    {
        if (currentState == SystemState::Paused)
        {
            return;
        }

        currentState = SystemState::Paused;
        OnPaused();
        SetEnabled(false);
    }

protected:
    virtual void OnActivated() {}
    virtual void OnDeactivated() {}
    virtual void OnPaused() {}

private:
    SystemState currentState;
};

}
