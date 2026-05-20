#pragma once

#include "Controller.h"
#include "Renderer.h"
#include "World.h"

enum class GameState {
    MainMenu,
    Playing,
    Quit,
};

class GameLoop {
public:
    void Run();

private:
    void HandleCommand(GameCommand command);
    ftxui::Element Render() const;

    GameState state_ = GameState::MainMenu;
    World world_;
    Renderer renderer_;
    Controller controller_;
};
