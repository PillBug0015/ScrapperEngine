#pragma once

#include "Component.h"
#include "Controllers/BattleController.h"
#include "Controllers/ChoiceController.h"
#include "Controllers/ControllerManager.h"
#include "Controllers/StoryController.h"
#include "Input/InputState.h"
#include "Views/CombatView.h"

namespace mud {
    class GameManager : public Component
    {
    private:
        static constexpr int ScreenWidth = 960;
        static constexpr int ScreenHeight = 540;

        ControllerManager controllerManager;
        BattleController battleController;
        ChoiceController choiceController;
        StoryController storyController;
        CombatView* combatView = nullptr;

        void LoadTestScene();
        InputState ReadInput() const;

    public:
        GameManager();

        void Start() override;
        void Update(float deltaTime) override;
    };
}
