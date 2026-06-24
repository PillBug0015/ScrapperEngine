#pragma once

#include <string>
#include <vector>
#include <memory>
#include "GameObject.h"

class Engine {
public:
    Engine(int width, int height, const std::string& title);
    virtual ~Engine();

    // Prevent copying
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize();
    void Shutdown();
    void Run();

    GameObject* Instantiate2D(Vector2 position = { 0.0f, 0.0f }, TransformComponent* parent = nullptr);
    GameObject* InstantiateRect(Vector2 position = { 0.0f, 0.0f }, Vector2 size = { 100.0f, 100.0f }, TransformComponent* parent = nullptr);

    // Manage GameObjects running in the Engine
    void AddGameObject(std::unique_ptr<GameObject> obj) {
        gameObjects.push_back(std::move(obj));
    }

    void ClearGameObjects() {
        gameObjects.clear();
    }

private:
    void Update(float deltaTime);
    void Draw();
    void RemoveDestroyedGameObjects();

    int windowWidth;
    int windowHeight;
    std::string windowTitle;
    bool running;

    std::vector<std::unique_ptr<GameObject>> gameObjects;
};
