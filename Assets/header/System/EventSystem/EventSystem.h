#pragma once

#include "GameDatabase/GameDatabase.h"
#include "System/GameSystem.h"

#include <string>

namespace MyMudGame {

    class GameManager;

    enum class EventState
    {
        Idle,
        Typing,
        Selecting
    };

    class EventSystem : public GameSystem
    {
    public:
        void Initialize(const GameDatabase* gameDatabase, GameManager* gameManager);

        bool StartEvent(const std::string& eventId);
        void Continue();
        void Update(float deltaTime) override;

        EventState GetEventState() const { return eventState; }

    protected:
        void OnActivated() override;
        void OnDeactivated() override;
        void OnPaused() override;

    private:
        const GameDatabase* database = nullptr;
        GameManager* gameManager = nullptr;
        EventState eventState = EventState::Idle;

        float navigateInterval = 0.2f;
        void OnNavigateInput();
        void OnCommitInput();
    };
}
