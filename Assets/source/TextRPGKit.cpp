#include "GameManager.h"
#include "EventComponents.h"
#include "EventSystem.h"
#include "StorySystem.h"
#include "CombatSystem.h"
#include "Controller.h"
#include "RenderSystem.h"
#include <raylib.h>
#include <raymath.h>
#include <iostream>

using namespace ScrapperEngine;

// ============================================================================
// 1. EVENT COMPONENTS IMPLEMENTATION
// ============================================================================

void TextEvent::OnStart(GameManager* game) {
    game->AddLog(text);
}

void TextEvent::OnUpdate(float deltaTime) {
    (void)deltaTime;
    // Continuation is handled by Controller: setting finished = true
}

void ChoiceEvent::OnStart(GameManager* game) {
    game->AddLog(text);
    for (size_t i = 0; i < options.size(); ++i) {
        game->AddLog("  [" + std::to_string(i + 1) + "] " + options[i].label);
    }
}

void CombatEvent::OnStart(GameManager* game) {
    auto enemy = std::make_unique<Mob>(enemyName);
    enemy->SetStat("MaxHP", enemyHp);
    enemy->SetStat("HP", enemyHp);
    enemy->SetStat("ATK", enemyAtk);
    enemy->SetStat("Gold", 20); // Reward gold

    game->GetCombatSystem().StartCombat(std::move(enemy), &game->GetPlayer());
    game->TransitionTo(GameState::COMBAT_MODE);
}

bool CombatEvent::IsFinished() const {
    // Finished when the GameManager returns to STORY_MODE (meaning combat ended)
    return true; 
}

void RewardEvent::OnStart(GameManager* game) {
    game->GetPlayer().ModifyStat(statKey, delta);
    if (delta >= 0) {
        game->AddLog(">> Obtained +" + std::to_string(delta) + " " + statKey + ".");
    } else {
        game->AddLog(">> Lost " + std::to_string(-delta) + " " + statKey + ".");
    }
}


// ============================================================================
// 2. EVENT SYSTEM IMPLEMENTATION
// ============================================================================

void EventSystem::Update(float deltaTime, GameManager* game) {
    if (!activeEvent && !eventQueue.empty()) {
        activeEvent = eventQueue.front().get();
        activeEvent->OnStart(game);
    }

    if (activeEvent) {
        activeEvent->OnUpdate(deltaTime);
        
        // If it's a CombatEvent, it is finished when GameManager transitions back to STORY_MODE.
        if (dynamic_cast<CombatEvent*>(activeEvent)) {
            if (game->GetState() == GameState::STORY_MODE) {
                NextEvent(game);
            }
        } else if (activeEvent->IsFinished()) {
            NextEvent(game);
        }
    }
}

void EventSystem::NextEvent(GameManager* game) {
    if (!eventQueue.empty()) {
        eventQueue.erase(eventQueue.begin());
    }
    
    if (eventQueue.empty()) {
        activeEvent = nullptr;
    } else {
        activeEvent = eventQueue.front().get();
        activeEvent->OnStart(game);
    }
}

void EventSystem::Draw(GameManager* game) {
    if (!activeEvent) return;

    activeEvent->OnDraw();

    // If it is a ChoiceEvent, submit choice buttons to RenderSystem
    if (auto* choice = dynamic_cast<ChoiceEvent*>(activeEvent)) {
        int numChoices = choice->options.size();
        float buttonWidth = 260.0f;
        float buttonHeight = 50.0f;
        float spacing = 25.0f;
        float totalWidth = (numChoices * buttonWidth) + ((numChoices - 1) * spacing);
        float startX = (1024.0f - totalWidth) / 2.0f;
        float yPos = 580.0f;

        for (int i = 0; i < numChoices; ++i) {
            Rectangle btnRect = { startX + i * (buttonWidth + spacing), yPos, buttonWidth, buttonHeight };
            bool hovered = CheckCollisionPointRec(GetMousePosition(), btnRect);

            // Submit Outer Black Border (Layer 9)
            RenderCommand borderCmd;
            borderCmd.type = RenderType::RECTANGLE;
            borderCmd.position = { btnRect.x + buttonWidth / 2.0f, btnRect.y + buttonHeight / 2.0f };
            borderCmd.size = { buttonWidth, buttonHeight };
            borderCmd.color = BLACK;
            borderCmd.layer = 9;
            RenderSystem::Instance().Submit(borderCmd);

            // Submit Inner Button Body (Layer 10)
            RenderCommand bodyCmd;
            bodyCmd.type = RenderType::RECTANGLE;
            bodyCmd.position = borderCmd.position;
            bodyCmd.size = { buttonWidth - 6.0f, buttonHeight - 6.0f };
            bodyCmd.color = hovered ? LIGHTGRAY : WHITE;
            bodyCmd.layer = 10;
            RenderSystem::Instance().Submit(bodyCmd);

            // Submit Button Text (Layer 11)
            std::string label = "[" + std::to_string(i + 1) + "] " + choice->options[i].label;
            int textWidth = MeasureText(label.c_str(), 16);
            
            RenderCommand textCmd;
            textCmd.type = RenderType::TEXT;
            textCmd.position = { btnRect.x + (buttonWidth - textWidth) / 2.0f, btnRect.y + 17.0f };
            textCmd.text = label;
            textCmd.color = BLACK;
            textCmd.fontSize = 16;
            textCmd.layer = 11;
            RenderSystem::Instance().Submit(textCmd);

            // Click interaction
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                choice->SelectOption(i, game);
            }
        }
    } else if (dynamic_cast<TextEvent*>(activeEvent)) {
        // Submit flashing continue prompt
        static float flashTimer = 0.0f;
        flashTimer += GetFrameTime();
        if (std::sin(flashTimer * 6.0f) > 0.0f) {
            RenderCommand promptCmd;
            promptCmd.type = RenderType::TEXT;
            promptCmd.position = { 700.0f, 610.0f };
            promptCmd.text = "Click or Press SPACE to continue...";
            promptCmd.color = DARKGRAY;
            promptCmd.fontSize = 16;
            promptCmd.layer = 11;
            RenderSystem::Instance().Submit(promptCmd);
        }
    }
}


// ============================================================================
// 3. STORY SYSTEM IMPLEMENTATION
// ============================================================================

void StorySystem::TriggerStory(const std::string& storyId, GameManager* game) {
    auto& ev = game->GetEventSystem();

    if (storyId == "intro") {
        game->ClearLogs();
        ev.QueueEvent(std::make_unique<TextEvent>("You wake up on the damp grass of Onett Forest. Your head thumps."));
        ev.QueueEvent(std::make_unique<TextEvent>("To your left, you hear the soothing roar of a magical waterfall."));
        ev.QueueEvent(std::make_unique<TextEvent>("To your right, deep growling echoes from the dark thicket."));
        
        std::vector<ChoiceOption> opts = {
            {"Explore Waterfall (Left)", [](GameManager* g) { g->GetStorySystem().TriggerStory("explore_left", g); }},
            {"Explore Thicket (Right)", [](GameManager* g) { g->GetStorySystem().TriggerStory("explore_right", g); }}
        };
        ev.QueueEvent(std::make_unique<ChoiceEvent>("Where will you venture?", opts));
    }
    else if (storyId == "explore_left") {
        ev.QueueEvent(std::make_unique<TextEvent>("You push through the vines toward the sparkling waterfall."));
        ev.QueueEvent(std::make_unique<TextEvent>("Behind the roaring water, you discover a glowing wooden chest."));
        ev.QueueEvent(std::make_unique<TextEvent>("You smash the lock and find an ancient, refreshing Herb."));
        ev.QueueEvent(std::make_unique<RewardEvent>("HP", 30));
        ev.QueueEvent(std::make_unique<RewardEvent>("Gold", 15));
        ev.QueueEvent(std::make_unique<TextEvent>("Feeling invigorated, you realize the only way forward is to face the threat."));

        std::vector<ChoiceOption> opts = {
            {"Go to the Thicket", [](GameManager* g) { g->GetStorySystem().TriggerStory("explore_right", g); }}
        };
        ev.QueueEvent(std::make_unique<ChoiceEvent>("Proceed to the dangerous right path?", opts));
    }
    else if (storyId == "explore_right") {
        ev.QueueEvent(std::make_unique<TextEvent>("You step into the shadowy thicket. The trees block out the sun."));
        ev.QueueEvent(std::make_unique<TextEvent>("Suddenly, a wild, snarling Onett Goblin leaps from the bushes."));
        
        ev.QueueEvent(std::make_unique<CombatEvent>("Onett Goblin", 50, 10));
        
        ev.QueueEvent(std::make_unique<TextEvent>("The Onett Goblin dissolves into sparkling stardust."));
        ev.QueueEvent(std::make_unique<TextEvent>("On the ground lies the legendary Forest Relic, pulsing with warm energy."));
        ev.QueueEvent(std::make_unique<TextEvent>("You hold the relic high. The forest shadows recede. Peace is restored."));
        ev.QueueEvent(std::make_unique<TextEvent>("Thank you for playing the TextRPGKit demo."));

        std::vector<ChoiceOption> opts = {
            {"Restart Adventure", [](GameManager* g) {
                g->TransitionTo(GameState::STORY_MODE);
                g->GetPlayer().SetStat("HP", g->GetPlayer().GetStat("MaxHP"));
                g->GetPlayer().SetStat("Gold", 0);
                g->GetStorySystem().TriggerStory("intro", g);
            }}
        };
        ev.QueueEvent(std::make_unique<ChoiceEvent>("Congratulations. You are a Hero.", opts));
    }
}


// ============================================================================
// 4. COMBAT SYSTEM IMPLEMENTATION
// ============================================================================

void CombatSystem::StartCombat(std::unique_ptr<Mob> enemyMob, Mob* playerMob) {
    enemy = std::move(enemyMob);
    player = playerMob;
    
    combatFinished = false;
    playerWon = false;
    isPlayerTurn = true;
    waitingForEnemy = false;
    selectedMenuIndex = 0;

    battleLogs.clear();
    AddBattleLog("A wild " + enemy->name + " blocks your path.");
    AddBattleLog("Battle Start. Choose an action.");
}

void CombatSystem::AddBattleLog(const std::string& log) {
    battleLogs.push_back(log);
    if (battleLogs.size() > 4) {
        battleLogs.erase(battleLogs.begin());
    }
}

void CombatSystem::PlayerAttack(GameManager* game) {
    (void)game;
    if (!isPlayerTurn || combatFinished) return;

    int dmg = player->GetStat("ATK") + GetRandomValue(1, 6);
    enemy->ModifyStat("HP", -dmg);
    AddBattleLog("You strike " + enemy->name + " for " + std::to_string(dmg) + " damage.");

    if (enemy->IsDead()) {
        combatFinished = true;
        playerWon = true;
        AddBattleLog("Victory. " + enemy->name + " has been defeated.");
        int goldGained = enemy->GetStat("Gold", 10);
        player->ModifyStat("Gold", goldGained);
        AddBattleLog("You looted " + std::to_string(goldGained) + " Gold.");
        turnTimer = 2.0f; // Wait 2 seconds before returning to story
        return;
    }

    isPlayerTurn = false;
    waitingForEnemy = true;
    turnTimer = 1.2f; // Delay before enemy attacks
}

void CombatSystem::PlayerHeal(GameManager* game) {
    (void)game;
    if (!isPlayerTurn || combatFinished) return;

    int healAmount = 25;
    player->ModifyStat("HP", healAmount);
    AddBattleLog("You cast Lifeup. Recovered " + std::to_string(healAmount) + " HP.");

    isPlayerTurn = false;
    waitingForEnemy = true;
    turnTimer = 1.2f;
}

void CombatSystem::PlayerRun(GameManager* game) {
    (void)game;
    if (!isPlayerTurn || combatFinished) return;

    if (GetRandomValue(0, 1) == 0) {
        AddBattleLog("You escaped from the battle.");
        combatFinished = true;
        playerWon = true;
        turnTimer = 1.5f;
    } else {
        AddBattleLog("You failed to escape.");
        isPlayerTurn = false;
        waitingForEnemy = true;
        turnTimer = 1.2f;
    }
}

void CombatSystem::EnemyTurn(GameManager* game) {
    (void)game;
    if (combatFinished) return;

    int dmg = enemy->GetStat("ATK") + GetRandomValue(-2, 3);
    if (dmg < 1) dmg = 1;

    player->ModifyStat("HP", -dmg);
    AddBattleLog(enemy->name + " strikes back. You take " + std::to_string(dmg) + " damage.");

    if (player->IsDead()) {
        combatFinished = true;
        playerWon = false;
        AddBattleLog("You collapse in defeat...");
        turnTimer = 2.0f;
        return;
    }

    isPlayerTurn = true;
    waitingForEnemy = false;
}

void CombatSystem::Update(float deltaTime, GameManager* game) {
    if (combatFinished) {
        turnTimer -= deltaTime;
        if (turnTimer <= 0.0f || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (playerWon) {
                game->TransitionTo(GameState::STORY_MODE);
            } else {
                game->TransitionTo(GameState::GAME_OVER);
            }
        }
        return;
    }

    if (waitingForEnemy) {
        turnTimer -= deltaTime;
        if (turnTimer <= 0.0f) {
            EnemyTurn(game);
        }
    }
}

void CombatSystem::Draw(GameManager* game) {
    // --- 1. Battle Frame Outline (Layer 0, 1) ---
    // Outer border (Gray)
    RenderCommand outFrame;
    outFrame.type = RenderType::RECTANGLE;
    outFrame.position = { 30.0f + 964.0f / 2.0f, 120.0f + 300.0f / 2.0f };
    outFrame.size = { 964.0f, 300.0f };
    outFrame.color = DARKGRAY;
    outFrame.layer = 0;
    RenderSystem::Instance().Submit(outFrame);

    // Inner panel (Black)
    RenderCommand inFrame = outFrame;
    inFrame.size = { 956.0f, 292.0f };
    inFrame.color = BLACK;
    inFrame.layer = 1;
    RenderSystem::Instance().Submit(inFrame);

    // --- 2. Psychedelic Mother-style background waves (Layer 2) ---
    static float waveTime = 0.0f;
    waveTime += GetFrameTime();
    for (int x = 40; x < 984; x += 8) {
        float y1 = 250.0f + std::sin(x * 0.03f + waveTime * 3.0f) * 50.0f;
        float y2 = 290.0f + std::cos(x * 0.02f - waveTime * 2.0f) * 40.0f;
        
        RenderCommand waveLine;
        waveLine.type = RenderType::LINE;
        waveLine.position = { (float)x, y1 };
        waveLine.endPosition = { (float)x + 4.0f, y2 };
        waveLine.color = MAROON;
        waveLine.thickness = 1.5f;
        waveLine.layer = 2;
        RenderSystem::Instance().Submit(waveLine);
    }

    // --- 3. Enemy Rendering (Retro Atari/NES Goblin) (Layer 3, 4) ---
    if (enemy) {
        // Head (Green Square)
        RenderCommand head;
        head.type = RenderType::RECTANGLE;
        head.position = { 512.0f, 260.0f };
        head.size = { 80.0f, 80.0f };
        head.color = LIME;
        head.layer = 3;
        RenderSystem::Instance().Submit(head);

        // Left ear (Green Line)
        RenderCommand earL;
        earL.type = RenderType::LINE;
        earL.position = { 472.0f, 250.0f };
        earL.endPosition = { 420.0f, 220.0f };
        earL.color = LIME;
        earL.thickness = 4.0f;
        earL.layer = 4;
        RenderSystem::Instance().Submit(earL);

        // Right ear (Green Line)
        RenderCommand earR = earL;
        earR.position = { 552.0f, 250.0f };
        earR.endPosition = { 604.0f, 220.0f };
        RenderSystem::Instance().Submit(earR);

        // Eyes (Red Squares)
        RenderCommand eyeL;
        eyeL.type = RenderType::RECTANGLE;
        eyeL.position = { 494.0f, 250.0f };
        eyeL.size = { 12.0f, 12.0f };
        eyeL.color = RED;
        eyeL.layer = 4;
        RenderSystem::Instance().Submit(eyeL);

        RenderCommand eyeR = eyeL;
        eyeR.position = { 530.0f, 250.0f };
        RenderSystem::Instance().Submit(eyeR);

        // Sinister mouth grin (Black line)
        RenderCommand mouth;
        mouth.type = RenderType::LINE;
        mouth.position = { 492.0f, 282.0f };
        mouth.endPosition = { 532.0f, 282.0f };
        mouth.color = BLACK;
        mouth.thickness = 3.0f;
        mouth.layer = 4;
        RenderSystem::Instance().Submit(mouth);

        // HP Text (Layer 5)
        std::string hpText = enemy->name + " (HP: " + std::to_string(enemy->GetStat("HP")) + "/" + std::to_string(enemy->GetStat("MaxHP")) + ")";
        int textWidth = MeasureText(hpText.c_str(), 18);
        
        RenderCommand nameCmd;
        nameCmd.type = RenderType::TEXT;
        nameCmd.position = { (1024.0f - textWidth) / 2.0f, 140.0f };
        nameCmd.text = hpText;
        nameCmd.color = WHITE;
        nameCmd.fontSize = 18;
        nameCmd.layer = 5;
        RenderSystem::Instance().Submit(nameCmd);

        // HP bar background (Layer 5)
        RenderCommand barBg;
        barBg.type = RenderType::RECTANGLE;
        barBg.position = { 512.0f, 176.0f };
        barBg.size = { 200.0f, 12.0f };
        barBg.color = DARKGRAY;
        barBg.layer = 5;
        RenderSystem::Instance().Submit(barBg);

        // HP bar fill (Layer 6)
        float hpPercent = (float)enemy->GetStat("HP") / enemy->GetStat("MaxHP");
        RenderCommand barFill;
        barFill.type = RenderType::RECTANGLE;
        // Adjust center based on percentage
        float fillWidth = 196.0f * hpPercent;
        barFill.position = { 512.0f - (196.0f - fillWidth) / 2.0f, 176.0f };
        barFill.size = { fillWidth, 8.0f };
        barFill.color = RED;
        barFill.layer = 6;
        RenderSystem::Instance().Submit(barFill);
    }

    // --- 4. Battle Log Box (Layer 8, 9, 10) ---
    // Outer border (White)
    RenderCommand logOut;
    logOut.type = RenderType::RECTANGLE;
    logOut.position = { 30.0f + 600.0f / 2.0f, 440.0f + 180.0f / 2.0f };
    logOut.size = { 600.0f, 180.0f };
    logOut.color = WHITE;
    logOut.layer = 8;
    RenderSystem::Instance().Submit(logOut);

    // Inner panel (Black)
    RenderCommand logIn = logOut;
    logIn.size = { 592.0f, 172.0f };
    logIn.color = BLACK;
    logIn.layer = 9;
    RenderSystem::Instance().Submit(logIn);

    // Render battle logs (Layer 10)
    for (size_t i = 0; i < battleLogs.size(); ++i) {
        RenderCommand logText;
        logText.type = RenderType::TEXT;
        logText.position = { 55.0f, 465.0f + (i * 30.0f) };
        logText.text = battleLogs[i];
        logText.color = WHITE;
        logText.fontSize = 16;
        logText.layer = 10;
        RenderSystem::Instance().Submit(logText);
    }

    // --- 5. JRPG Battle Menu Box (Layer 8, 9, 10, 11) ---
    // Outer border (White)
    RenderCommand menuOut;
    menuOut.type = RenderType::RECTANGLE;
    menuOut.position = { 650.0f + 344.0f / 2.0f, 440.0f + 180.0f / 2.0f };
    menuOut.size = { 344.0f, 180.0f };
    menuOut.color = WHITE;
    menuOut.layer = 8;
    RenderSystem::Instance().Submit(menuOut);

    // Inner panel (Black)
    RenderCommand menuIn = menuOut;
    menuIn.size = { 336.0f, 172.0f };
    menuIn.color = BLACK;
    menuIn.layer = 9;
    RenderSystem::Instance().Submit(menuIn);

    // Title (Layer 10)
    RenderCommand titleCmd;
    titleCmd.type = RenderType::TEXT;
    titleCmd.position = { 740.0f, 460.0f };
    titleCmd.text = "COMMANDS";
    titleCmd.color = GOLD;
    titleCmd.fontSize = 18;
    titleCmd.layer = 10;
    RenderSystem::Instance().Submit(titleCmd);

    // Menu options
    std::vector<std::string> commands = { "[1] Attack", "[2] Lifeup (Heal)", "[3] Run" };
    for (int i = 0; i < 3; ++i) {
        Rectangle btnRect = { 670.0f, 495.0f + (i * 35), 304.0f, 30.0f };
        bool hovered = CheckCollisionPointRec(GetMousePosition(), btnRect);

        Color textColor = WHITE;
        if (hovered || selectedMenuIndex == i) {
            // Highlighting (Layer 10)
            RenderCommand highCmd;
            highCmd.type = RenderType::RECTANGLE;
            highCmd.position = { btnRect.x + btnRect.width / 2.0f, btnRect.y + btnRect.height / 2.0f };
            highCmd.size = { btnRect.width, btnRect.height };
            highCmd.color = DARKGRAY;
            highCmd.layer = 10;
            RenderSystem::Instance().Submit(highCmd);

            textColor = YELLOW;
        }

        // Text item (Layer 11)
        RenderCommand itemCmd;
        itemCmd.type = RenderType::TEXT;
        itemCmd.position = { btnRect.x + 20.0f, btnRect.y + 7.0f };
        itemCmd.text = commands[i];
        itemCmd.color = textColor;
        itemCmd.fontSize = 16;
        itemCmd.layer = 11;
        RenderSystem::Instance().Submit(itemCmd);

        // Click detection
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedMenuIndex = i;
            if (i == 0) PlayerAttack(game);
            else if (i == 1) PlayerHeal(game);
            else if (i == 2) PlayerRun(game);
        }
    }
}


// ============================================================================
// 5. CONTROLLER IMPLEMENTATION
// ============================================================================

void Controller::HandleInput(float deltaTime, GameManager* game) {
    (void)deltaTime;

    if (game->GetState() == GameState::STORY_MODE) {
        EventComponent* active = game->GetEventSystem().GetActiveEvent();
        if (!active) return;

        if (auto* textEvent = dynamic_cast<TextEvent*>(active)) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                textEvent->finished = true;
            }
        }
        else if (auto* choiceEvent = dynamic_cast<ChoiceEvent*>(active)) {
            if (IsKeyPressed(KEY_ONE)) choiceEvent->SelectOption(0, game);
            if (IsKeyPressed(KEY_TWO)) choiceEvent->SelectOption(1, game);
            if (IsKeyPressed(KEY_THREE)) choiceEvent->SelectOption(2, game);
        }
    }
    else if (game->GetState() == GameState::COMBAT_MODE) {
        CombatSystem& combat = game->GetCombatSystem();
        if (IsKeyPressed(KEY_ONE)) combat.PlayerAttack(game);
        if (IsKeyPressed(KEY_TWO)) combat.PlayerHeal(game);
        if (IsKeyPressed(KEY_THREE)) combat.PlayerRun(game);
    }
    else if (game->GetState() == GameState::GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            game->TransitionTo(GameState::STORY_MODE);
            game->GetPlayer().SetStat("HP", game->GetPlayer().GetStat("MaxHP"));
            game->GetPlayer().SetStat("Gold", 0);
            game->GetStorySystem().TriggerStory("intro", game);
        }
    }
}


// ============================================================================
// 6. GAME MANAGER IMPLEMENTATION
// ============================================================================

GameManager::GameManager() {
    player.name = "Ness";
    player.SetStat("MaxHP", 80);
    player.SetStat("HP", 80);
    player.SetStat("ATK", 12);
    player.SetStat("Level", 1);
    player.SetStat("Gold", 0);
}

void GameManager::Start() {
    ClearLogs();
    storySystem.TriggerStory("intro", this);
}

void GameManager::TransitionTo(GameState newState) {
    state = newState;
    if (state == GameState::COMBAT_MODE) {
        AddLog(">> Triggers JRPG Battle Mode!");
    } else if (state == GameState::STORY_MODE) {
        AddLog(">> Returns to Story Mode.");
    }
}

void GameManager::AddLog(const std::string& log) {
    gameLogs.push_back(log);
    if (gameLogs.size() > 15) {
        gameLogs.erase(gameLogs.begin());
    }
}

void GameManager::ClearLogs() {
    gameLogs.clear();
}

void GameManager::Update(float deltaTime) {
    controller.HandleInput(deltaTime, this);

    if (state == GameState::STORY_MODE) {
        eventSystem.Update(deltaTime, this);
    } else if (state == GameState::COMBAT_MODE) {
        combatSystem.Update(deltaTime, this);
    }
}

void GameManager::SubmitRenderCommand() {
    if (state == GameState::STORY_MODE) {
        DrawHeader();
        DrawLogs();
        eventSystem.Draw(this);
    } else if (state == GameState::COMBAT_MODE) {
        DrawHeader();
        combatSystem.Draw(this);
    } else if (state == GameState::GAME_OVER) {
        DrawGameOver();
    }
}

void GameManager::DrawHeader() {
    // Submit HUD Outer Border (White, Layer 8)
    RenderCommand hudOut;
    hudOut.type = RenderType::RECTANGLE;
    hudOut.position = { 30.0f + 964.0f / 2.0f, 20.0f + 70.0f / 2.0f };
    hudOut.size = { 964.0f, 70.0f };
    hudOut.color = WHITE;
    hudOut.layer = 8;
    RenderSystem::Instance().Submit(hudOut);

    // Submit HUD Inner Panel (Black, Layer 9)
    RenderCommand hudIn = hudOut;
    hudIn.size = { 956.0f, 62.0f };
    hudIn.color = BLACK;
    hudIn.layer = 9;
    RenderSystem::Instance().Submit(hudIn);

    // Submit texts (Layer 10)
    RenderCommand nameCmd;
    nameCmd.type = RenderType::TEXT;
    nameCmd.position = { 60.0f, 45.0f };
    nameCmd.text = player.name;
    nameCmd.color = GOLD;
    nameCmd.fontSize = 22;
    nameCmd.layer = 10;
    RenderSystem::Instance().Submit(nameCmd);

    RenderCommand hpCmd;
    hpCmd.type = RenderType::TEXT;
    hpCmd.position = { 250.0f, 47.0f };
    hpCmd.text = "HP: " + std::to_string(player.GetStat("HP")) + "/" + std::to_string(player.GetStat("MaxHP"));
    hpCmd.color = WHITE;
    hpCmd.fontSize = 18;
    hpCmd.layer = 10;
    RenderSystem::Instance().Submit(hpCmd);

    RenderCommand lvlCmd;
    lvlCmd.type = RenderType::TEXT;
    lvlCmd.position = { 500.0f, 47.0f };
    lvlCmd.text = "LV: " + std::to_string(player.GetStat("Level"));
    lvlCmd.color = WHITE;
    lvlCmd.fontSize = 18;
    lvlCmd.layer = 10;
    RenderSystem::Instance().Submit(lvlCmd);

    RenderCommand goldCmd;
    goldCmd.type = RenderType::TEXT;
    goldCmd.position = { 750.0f, 47.0f };
    goldCmd.text = "GOLD: " + std::to_string(player.GetStat("Gold"));
    goldCmd.color = GREEN;
    goldCmd.fontSize = 18;
    goldCmd.layer = 10;
    RenderSystem::Instance().Submit(goldCmd);
}

void GameManager::DrawLogs() {
    // Submit Log Panel Outer Border (White, Layer 8)
    RenderCommand logOut;
    logOut.type = RenderType::RECTANGLE;
    logOut.position = { 30.0f + 964.0f / 2.0f, 110.0f + 450.0f / 2.0f };
    logOut.size = { 964.0f, 450.0f };
    logOut.color = WHITE;
    logOut.layer = 8;
    RenderSystem::Instance().Submit(logOut);

    // Submit Log Panel Inner Panel (Black, Layer 9)
    RenderCommand logIn = logOut;
    logIn.size = { 956.0f, 442.0f };
    logIn.color = BLACK;
    logIn.layer = 9;
    RenderSystem::Instance().Submit(logIn);

    // Submit narration history logs (Layer 10)
    for (size_t i = 0; i < gameLogs.size(); ++i) {
        Color logColor = RAYWHITE;
        if (gameLogs[i].rfind(">>", 0) == 0) {
            logColor = GREEN;
        } else if (gameLogs[i].rfind("  [", 0) == 0) {
            logColor = GOLD;
        }

        RenderCommand textCmd;
        textCmd.type = RenderType::TEXT;
        textCmd.position = { 60.0f, 135.0f + (i * 26.0f) };
        textCmd.text = gameLogs[i];
        textCmd.color = logColor;
        textCmd.fontSize = 16;
        textCmd.layer = 10;
        RenderSystem::Instance().Submit(textCmd);
    }
}

void GameManager::DrawGameOver() {
    // GameOver title text (Layer 10)
    int titleWidth = MeasureText("GAME OVER", 48);
    RenderCommand titleCmd;
    titleCmd.type = RenderType::TEXT;
    titleCmd.position = { (1024.0f - titleWidth) / 2.0f, 280.0f };
    titleCmd.text = "GAME OVER";
    titleCmd.color = RED;
    titleCmd.fontSize = 48;
    titleCmd.layer = 10;
    RenderSystem::Instance().Submit(titleCmd);

    // Description text (Layer 10)
    std::string desc = "Your HP reached 0. You collapsed in the woods.";
    int descWidth = MeasureText(desc.c_str(), 20);
    RenderCommand descCmd;
    descCmd.type = RenderType::TEXT;
    descCmd.position = { (1024.0f - descWidth) / 2.0f, 380.0f };
    descCmd.text = desc;
    descCmd.color = LIGHTGRAY;
    descCmd.fontSize = 20;
    descCmd.layer = 10;
    RenderSystem::Instance().Submit(descCmd);

    // Flashing restart prompt (Layer 10)
    static float flashTimer = 0.0f;
    flashTimer += GetFrameTime();
    if (std::sin(flashTimer * 5.0f) > 0.0f) {
        std::string restart = "Press SPACE, ENTER or CLICK to wake up again...";
        int restartWidth = MeasureText(restart.c_str(), 18);
        
        RenderCommand restartCmd;
        restartCmd.type = RenderType::TEXT;
        restartCmd.position = { (1024.0f - restartWidth) / 2.0f, 480.0f };
        restartCmd.text = restart;
        restartCmd.color = GOLD;
        restartCmd.fontSize = 18;
        restartCmd.layer = 10;
        RenderSystem::Instance().Submit(restartCmd);
    }
}
