#pragma once

#include <ftxui/component/event.hpp>

enum class GameCommand {
    None,
    StartGame,
    Quit,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
};

class Controller {
public:
    GameCommand ToCommand(const ftxui::Event& event) const;
};
