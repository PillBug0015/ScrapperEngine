#include "Controller.h"

GameCommand Controller::ToCommand(const ftxui::Event& event) const {
    if (event == ftxui::Event::Character("1")) {
        return GameCommand::StartGame;
    }

    if (event == ftxui::Event::Character("3") || event == ftxui::Event::Character("q") ||
        event == ftxui::Event::Character("Q") || event == ftxui::Event::Escape) {
        return GameCommand::Quit;
    }

    if (event == ftxui::Event::Character("w") || event == ftxui::Event::Character("W") ||
        event == ftxui::Event::ArrowUp) {
        return GameCommand::MoveUp;
    }

    if (event == ftxui::Event::Character("s") || event == ftxui::Event::Character("S") ||
        event == ftxui::Event::ArrowDown) {
        return GameCommand::MoveDown;
    }

    if (event == ftxui::Event::Character("a") || event == ftxui::Event::Character("A") ||
        event == ftxui::Event::ArrowLeft) {
        return GameCommand::MoveLeft;
    }

    if (event == ftxui::Event::Character("d") || event == ftxui::Event::Character("D") ||
        event == ftxui::Event::ArrowRight) {
        return GameCommand::MoveRight;
    }

    return GameCommand::None;
}
