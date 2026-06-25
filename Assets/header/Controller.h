#pragma once

class GameManager;

class Controller {
public:
    Controller() = default;
    ~Controller() = default;

    void HandleInput(float deltaTime, GameManager* game);
};
