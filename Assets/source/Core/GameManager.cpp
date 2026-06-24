#include "GameManager.h"

#include <memory>
#include <raylib.h>
#include <string>

#include "Data/EntityData.h"
#include "Data/GameDatabase.h"
#include "Data/StatKey.h"
#include "Events/BattleStartComponent.h"
#include "Events/ChoiceComponent.h"
#include "Events/Event.h"
#include "Events/Scene.h"
#include "Events/TextComponent.h"
#include "Systems/BattleSystem.h"
#include "Systems/ChoiceSystem.h"
#include "Systems/EventSystem.h"
#include "Data/RenderData.h"
#include "Systems/TextSystem.h"

namespace mud {
    GameManager::GameManager()
    {
        controllerManager.Register(battleController);
        controllerManager.Register(choiceController);
        controllerManager.Register(storyController);
    }

    void GameManager::Start()
    {
        combatView = owner->AddComponent<CombatView>();
        combatView->SetEnabled(false);

        LoadTestScene();
    }

    void GameManager::LoadTestScene()
    {
        GameDatabase::Instance().Clear();
        TextSystem::Instance().Clear();
        ChoiceSystem::Instance().Close();
        BattleSystem::Instance().EndBattle();
        EventSystem::Instance().Clear();

        EntityData playerData;
        playerData.ID = "player";
        playerData.DisplayName = "Traveler";
        playerData.Stats = {
            {StatKey::Hp, 30},
            {StatKey::MaxHp, 30},
            {StatKey::Attack, 6},
            {StatKey::Defense, 2},
            {StatKey::Speed, 4}
        };
        playerData.ActionIDs = {"Attack", "Guard", "Item", "Run"};

        EntityData stationEchoData;
        stationEchoData.ID = "station_echo";
        stationEchoData.DisplayName = "Station Echo";
        stationEchoData.Stats = {
            {StatKey::Hp, 18},
            {StatKey::MaxHp, 18},
            {StatKey::Attack, 4},
            {StatKey::Defense, 1},
            {StatKey::Speed, 2}
        };
        stationEchoData.ActionIDs = {"Whisper"};

        EntityData straySignalData;
        straySignalData.ID = "stray_signal";
        straySignalData.DisplayName = "Stray Signal";
        straySignalData.Stats = {
            {StatKey::Hp, 12},
            {StatKey::MaxHp, 12},
            {StatKey::Attack, 3},
            {StatKey::Defense, 0},
            {StatKey::Speed, 5}
        };
        straySignalData.ActionIDs = {"Static"};

        GameDatabase::Instance().AddEntityData(playerData);
        GameDatabase::Instance().AddEntityData(stationEchoData);
        GameDatabase::Instance().AddEntityData(straySignalData);

        auto scene = std::make_unique<Scene>();
        scene->StartEventID = "start";

        auto startEvent = std::make_unique<Event>();
        startEvent->UniqueID = "start";

        auto startText = std::make_unique<TextComponent>();
        startText->content = "Snow falls over the old station.";
        startEvent->Components.push_back(std::move(startText));

        auto startText2 = std::make_unique<TextComponent>();
        startText2->content = "The platform lights blink in the distance.";
        startEvent->Components.push_back(std::move(startText2));

        auto startChoice = std::make_unique<ChoiceComponent>();
        startChoice->Choices.push_back({"Enter the station", "station"});
        startChoice->Choices.push_back({"Turn back", "end"});
        startEvent->Components.push_back(std::move(startChoice));

        auto stationEvent = std::make_unique<Event>();
        stationEvent->UniqueID = "station";

        auto stationText = std::make_unique<TextComponent>();
        stationText->content = "Inside, an old timetable hums like it remembers you.";
        stationEvent->Components.push_back(std::move(stationText));

        auto battleStart = std::make_unique<BattleStartComponent>();
        battleStart->PlayerEntityIDs.push_back("player");
        battleStart->EnemyEntityIDs.push_back("station_echo");
        battleStart->EnemyEntityIDs.push_back("stray_signal");
        stationEvent->Components.push_back(std::move(battleStart));

        auto endEvent = std::make_unique<Event>();
        endEvent->UniqueID = "end";

        auto endText = std::make_unique<TextComponent>();
        endText->content = "You decide the night can wait.";
        endEvent->Components.push_back(std::move(endText));

        scene->AddEvent(std::move(startEvent));
        scene->AddEvent(std::move(stationEvent));
        scene->AddEvent(std::move(endEvent));

        EventSystem::Instance().LoadScene(std::move(scene));
    }

    InputState GameManager::ReadInput() const
    {
        InputState input;
        input.Up = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        input.Down = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        input.Left = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        input.Right = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        input.Confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z);
        input.Cancel = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE);
        input.Menu = IsKeyPressed(KEY_ESCAPE);
        return input;
    }

    void GameManager::Update(float deltaTime)
    {
        UpdateContext context;
        context.DeltaTime = deltaTime;
        context.Input = ReadInput();
        controllerManager.Update(context);

        const bool battleActive = BattleSystem::Instance().IsActive();
        if (combatView != nullptr)
        {
            combatView->SetEnabled(battleActive);

            if (battleActive)
            {
                combatView->SetData(BattleSystem::Instance().GetRenderData());
            }
            else
            {
                combatView->ClearData();
            }
        }
    }


}
