#include "Core/Engine.h"
#include "Rendering/RenderSystem.h"
#include "Physics/PhysicsWorld.h"
#include <algorithm>
#include <raylib.h>


namespace ScrapperEngine {
Engine::Engine(int width, int height, const std::string& title, int targetWidth, int targetHeight)
    : windowWidth(width), windowHeight(height), windowTitle(title), running(false) {
    RenderSystem::Instance().ConfigureTargetResolution(targetWidth, targetHeight);
}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Initialize() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, windowTitle.c_str());
    SetTargetFPS(60);
    running = IsWindowReady();

    if (running) {
        RenderSystem::Instance().Initialize();

        // --- 글로벌 기본 한글 폰트 자동 로드 및 연동 ---
        const char* fontPath = "Fonts/default.ttf";
        if (FileExists(fontPath)) {
            std::vector<int> codepoints;
            for (int i = 32; i < 127; ++i) {
                codepoints.push_back(i);
            }
            
            for (int i = 0xAC00; i <= 0xD7A3; ++i) {
                codepoints.push_back(i);
            }
            
            // 3. Raylib 폰트 확장 로드 함수 실행
            loadedFont = LoadFontEx(fontPath, 32, codepoints.data(), static_cast<int>(codepoints.size()));
            
            // 4. 렌더 시스템에 글로벌 폰트 등록
            RenderSystem::Instance().SetDefaultFont(loadedFont);
        }

        PhysicsWorld::Instance().Initialize();
    }

    return running;
}

void Engine::Shutdown() {
    // 1. First, clear all game objects while the OpenGL context and physics world are still alive
    ClearGameObjects();
    PhysicsWorld::Instance().Shutdown();

    // 2. 글로벌 폰트 해제 (OpenGL 컨텍스트 소멸 전에 수행해야 함)
    if (loadedFont.texture.id > 0) {
        UnloadFont(loadedFont);
        loadedFont = {};
        RenderSystem::Instance().SetDefaultFont({});
    }

    // 3. Unload render targets before closing the window
    RenderSystem::Instance().Shutdown();

    // 4. Finally, close the window and destroy the OpenGL context
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

        RenderSystem::Instance().RenderFrame();
    }
}
GameObject* Engine::Instantiate() {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    AddGameObject(std::move(obj));
    return ptr;
}


GameObject* Engine::Instantiate2D(Vector2 position, Transform2DBase* parent) {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    Transform2D* transform = ptr->AddComponent<Transform2D>();
    transform->SetLocalPosition(position);
    transform->SetParent(parent);

    AddGameObject(std::move(obj));
    return ptr;
}

GameObject* Engine::InstantiateRect(Vector2 position, Vector2 size, Transform2DBase* parent) {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    RectTransform* transform = ptr->AddComponent<RectTransform>(size);
    transform->SetLocalPosition(position);
    transform->SetParent(parent);

    AddGameObject(std::move(obj));
    return ptr;
}

GameObject* Engine::Instantiate3D(Vector3 position, Transform3D* parent) {
    auto obj = std::make_unique<GameObject>();
    GameObject* ptr = obj.get();

    Transform3D* transform = ptr->AddComponent<Transform3D>();
    transform->SetLocalPosition(position);
    transform->SetParent(parent);

    AddGameObject(std::move(obj));
    return ptr;
}

void Engine::Update(float deltaTime) {
    updating = true;

    // Index-based loop: objects spawned during Update land in pendingGameObjects,
    // so this container is never reallocated mid-iteration.
    for (std::size_t i = 0; i < gameObjects.size(); ++i) {
        GameObject* obj = gameObjects[i].get();
        if (obj && obj->IsActiveInHierarchy()) {
            obj->Update(deltaTime);
        }
    }

    // Game logic has applied its forces; advance physics and write poses back to transforms.
    PhysicsWorld::Instance().Step(deltaTime);

    for (std::size_t i = 0; i < gameObjects.size(); ++i) {
        GameObject* obj = gameObjects[i].get();
        if (obj && obj->IsActiveInHierarchy()) {
            obj->LateUpdate(deltaTime);
        }
    }

    updating = false;

    // Objects created this frame join the simulation from the next frame on.
    for (auto& pending : pendingGameObjects) {
        gameObjects.push_back(std::move(pending));
    }
    pendingGameObjects.clear();

    RemoveDestroyedGameObjects();
}

void Engine::RemoveDestroyedGameObjects() {
    gameObjects.erase(
        std::remove_if(gameObjects.begin(), gameObjects.end(), [](const std::unique_ptr<GameObject>& obj) {
            return obj == nullptr || obj->IsDestroyed();
        }),
        gameObjects.end()
    );
}

}
