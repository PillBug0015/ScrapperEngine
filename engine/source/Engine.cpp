#include "Engine.h"
#include "RenderSystem.h"
#include <algorithm>
#include <raylib.h>

Engine::Engine(int width, int height, const std::string& title)
    : windowWidth(width), windowHeight(height), windowTitle(title), running(false) {}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Initialize() {
    InitWindow(windowWidth, windowHeight, windowTitle.c_str());
    SetTargetFPS(60);
    running = IsWindowReady();
    return running;
}

void Engine::Shutdown() {
    if (IsWindowReady()) {
        CloseWindow();
    }
    running = false;
}

void Engine::Run() {
    if (!running) return;

    while (!WindowShouldClose() && running) {
        float deltaTime = GetFrameTime();
        Update(deltaTime);

        BeginDrawing();
        ClearBackground(BLACK); // Black background as default for JRPG/Text RPG

        Draw();

        EndDrawing();
    }
}

GameObject* Engine::Instantiate2D(Vector2 position, TransformComponent* parent) {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    Transform2D* transform = ptr->AddComponent<Transform2D>();
    transform->SetLocalPosition(position);
    transform->SetParent(parent);

    AddGameObject(std::move(obj));
    return ptr;
}

GameObject* Engine::InstantiateRect(Vector2 position, Vector2 size, TransformComponent* parent) {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    RectTransform* transform = ptr->AddComponent<RectTransform>(size);
    transform->SetLocalPosition(position);
    transform->SetParent(parent);

    AddGameObject(std::move(obj));
    return ptr;
}

void Engine::Update(float deltaTime) {
    for (auto& obj : gameObjects) {
        if (obj && obj->IsActiveInHierarchy()) {
            obj->Update(deltaTime);
        }
    }

    RemoveDestroyedGameObjects();
}

void Engine::Draw() {
    // All rendering is now managed by registered renderers inside RenderSystem.
    // There is no need to traverse GameObjects to draw them.
    RenderSystem::Instance().RenderAll();
}

void Engine::RemoveDestroyedGameObjects() {
    gameObjects.erase(
        std::remove_if(gameObjects.begin(), gameObjects.end(), [](const std::unique_ptr<GameObject>& obj) {
            return obj == nullptr || obj->IsDestroyed();
        }),
        gameObjects.end()
    );
}
