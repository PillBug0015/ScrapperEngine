#pragma once

#include <raylib.h>
#include <vector>
#include <string>


namespace ScrapperEngine {
enum class RenderType {
    RECTANGLE,
    TEXTURE,
    TEXT,
    LINE
};

enum class RenderBucket {
    None,
    World,
    UI
};

struct RenderCommand {
    RenderType type = RenderType::RECTANGLE;
    
    // Transform
    Vector2 position = { 0.0f, 0.0f };
    float rotation = 0.0f;
    Vector2 scale = { 1.0f, 1.0f };
    
    // Styling
    Color color = WHITE;
    int layer = 0; // Lower numbers drawn first, higher numbers drawn on top
    
    // Rectangle specific
    Vector2 size = { 0.0f, 0.0f };

    // Texture specific
    Texture2D texture = {};
    Rectangle sourceRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    // Text specific
    std::string text = "";
    int fontSize = 20;
    
    // Line specific
    Vector2 endPosition = { 0.0f, 0.0f };
    float thickness = 1.0f;

    // Pivot/Anchor ratio (0.0 to 1.0, default 0.5 for center)
    Vector2 pivot = { 0.5f, 0.5f };
};

// Forward declaration to avoid circular dependency
class Renderer;
class CameraComponent;

class RenderSystem {
public:
    static RenderSystem& Instance() {
        static RenderSystem instance;
        return instance;
    }

    void Submit(const RenderCommand& cmd);

    void Clear() {
        worldCommands.clear();
        uiCommands.clear();
    }

    // Registration Methods
    void RegisterRenderer(Renderer* r);
    void UnregisterRenderer(Renderer* r);
    void RegisterCamera(CameraComponent* camera);
    void UnregisterCamera(CameraComponent* camera);

    // Main Render Method
    void RenderAll();

    // 글로벌 기본 한글 폰트 연동용 메서드 추가
    void SetDefaultFont(Font font) { defaultFont = font; }
    Font GetDefaultFont() const { return defaultFont; }

private:
    std::vector<RenderCommand> worldCommands;
    std::vector<RenderCommand> uiCommands;
    std::vector<Renderer*> worldRenderers;
    std::vector<Renderer*> uiRenderers;
    std::vector<CameraComponent*> registeredCameras;
    std::vector<RenderCommand>* activeCommandQueue = nullptr;
    
    Font defaultFont = {}; // 글로벌 기본 폰트 객체 보관용

    RenderSystem() = default;
    ~RenderSystem() = default;
    
    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;

    void RegisterWorldRenderer(Renderer* r);
    void RegisterUIRenderer(Renderer* r);
    void UnregisterFromBucket(Renderer* r, std::vector<Renderer*>& renderers);

    void CollectCommands(const std::vector<Renderer*>& renderers, std::vector<RenderCommand>& commands);
    void ExecuteCommands(const std::vector<RenderCommand>& cmds);
};

}
