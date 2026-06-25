#include "Core/Engine.h"
#include "Rendering/RenderSystem.h"
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
            // 1. 기본 영문/ASCII 영역 수집 (32 ~ 126)
            for (int i = 32; i < 127; ++i) {
                codepoints.push_back(i);
            }
            // 2. 한글 음절 전체 영역 수집 (0xAC00 ~ 0xD7A3)
            for (int i = 0xAC00; i <= 0xD7A3; ++i) {
                codepoints.push_back(i);
            }
            
            // 3. Raylib 폰트 확장 로드 함수 실행
            loadedFont = LoadFontEx(fontPath, 32, codepoints.data(), static_cast<int>(codepoints.size()));
            
            // 4. 렌더 시스템에 글로벌 폰트 등록
            RenderSystem::Instance().SetDefaultFont(loadedFont);
        }
    }

    return running;
}

void Engine::Shutdown() {
    // 1. First, clear all game objects while the OpenGL context is still alive
    ClearGameObjects();

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
    return ptr;
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

void Engine::RemoveDestroyedGameObjects() {
    gameObjects.erase(
        std::remove_if(gameObjects.begin(), gameObjects.end(), [](const std::unique_ptr<GameObject>& obj) {
            return obj == nullptr || obj->IsDestroyed();
        }),
        gameObjects.end()
    );
}

}
