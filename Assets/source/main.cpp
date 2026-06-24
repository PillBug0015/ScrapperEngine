#include "Scrapper.h"
#include "GameDatabase.h"
#include <iostream>

int main()
{
    // 1. 데이터베이스 초기화 및 데이터 로딩 테스트
    MyMudGame::GameDatabase db;
    std::cout << "[DEBUG] Loading database from 'data'..." << std::endl;
    if (db.LoadDatabase("data"))
    {
        std::cout << "[DEBUG] Database loaded successfully." << std::endl;
        std::cout << " - Entities: " << db.GetEntityCount() << " loaded." << std::endl;
        std::cout << " - Items: " << db.GetItemCount() << " loaded." << std::endl;
        std::cout << " - Actions: " << db.GetActionCount() << " loaded." << std::endl;

        // 개별 로드 검증
        const auto* player = db.GetEntityData("player");
        if (player) {
            std::cout << " - Found Entity 'player': Name=" << player->displayInitialName << std::endl;
        }

        // 복합(Mixed) 로드 검증 (goblin.json)
        const auto* boss = db.GetEntityData("goblin_boss");
        if (boss) {
            std::cout << " - Found Entity 'goblin_boss': Name=" << boss->displayInitialName << std::endl;
        }

        const auto* smash = db.GetActionData("boss_smash");
        if (smash) {
            std::cout << " - Found Action 'boss_smash': Name=" << smash->displayInitialName 
                      << ", MPCost=" << smash->mpCost << ", Damage=" << smash->damage << std::endl;
        }
    }
    else
    {
        std::cerr << "[DEBUG] Failed to load database." << std::endl;
    }

    // 2. 엔진 실행 및 렌더링
    ScrapperEngine::Engine engine(960, 540, "MyMudGame - Scrapper Engine", 960, 540);

    if (engine.Initialize())
    {
        // 버그 수정: engine.Run() 호출 전에 게임 오브젝트를 인스턴스화해야 화면에 렌더링됩니다.
        Vector2 position = {960 * 0.5f, 540 * 0.5f};
        ScrapperEngine::GameObject* title = engine.InstantiateRect(position);
        title->AddComponent<ScrapperEngine::TextRenderer>("Bruh");

        engine.Run();
    }
}
