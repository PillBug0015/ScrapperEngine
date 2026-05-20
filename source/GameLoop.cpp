#include "GameLoop.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

void GameLoop::Run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::TerminalOutput();

    auto component = CatchEvent(ftxui::Renderer([&] {
        return Render();
    }), [&](Event event) {
        const GameCommand command = controller_.ToCommand(event);
        HandleCommand(command);

        if (state_ == GameState::Quit) {
            screen.ExitLoopClosure()();
        }

        return command != GameCommand::None;
    });

    screen.Loop(component);
}

void GameLoop::HandleCommand(GameCommand command) {
    switch (command) {
    case GameCommand::StartGame:
        if (state_ == GameState::MainMenu) {
            state_ = GameState::Playing;
        }
        break;
    case GameCommand::Quit:
        state_ = GameState::Quit;
        break;
    case GameCommand::MoveUp:
        if (state_ == GameState::Playing) {
            world_.TryMoveEntity(world_.PlayerId(), 0, -1);
        }
        break;
    case GameCommand::MoveDown:
        if (state_ == GameState::Playing) {
            world_.TryMoveEntity(world_.PlayerId(), 0, 1);
        }
        break;
    case GameCommand::MoveLeft:
        if (state_ == GameState::Playing) {
            world_.TryMoveEntity(world_.PlayerId(), -1, 0);
        }
        break;
    case GameCommand::MoveRight:
        if (state_ == GameState::Playing) {
            world_.TryMoveEntity(world_.PlayerId(), 1, 0);
        }
        break;
    case GameCommand::None:
    default:
        break;
    }
}

ftxui::Element GameLoop::Render() const {
    if (state_ == GameState::Playing) {
        return renderer_.RenderWorld(world_);
    }

    return renderer_.RenderMenu();
}
