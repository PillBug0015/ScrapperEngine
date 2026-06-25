#include "Scrapper.h"
#include "Core/GameManager.h"
#include "System/CombatSystem/CombatSystem.h"
#include "System/EventSystem/EventSystem.h"

int main()
{
    ScrapperEngine::Engine engine(960, 540, "MyMudGame - Scrapper Engine", 960, 540);

    if (engine.Initialize())
    {
        ScrapperEngine::GameObject* root = engine.Instantiate2D();
        auto* eventSystem = root->AddComponent<MyMudGame::EventSystem>();
        auto* combatSystem = root->AddComponent<MyMudGame::CombatSystem>();
        root->AddComponent<MyMudGame::GameManager>(eventSystem, combatSystem);

        engine.Run();
    }

    return 0;
}
