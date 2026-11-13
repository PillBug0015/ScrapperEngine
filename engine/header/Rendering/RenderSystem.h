#pragma once

#include <raylib.h>
#include <array>
#include <cstddef>
#include <string>
#include <vector>


namespace ScrapperEngine {
enum class RenderType {
    RECTANGLE,
    TEXTURE,
    TEXT,
    LINE
};

enum class RenderBucket {
    None,
    World2D,
    World3D,
    UI
};

enum class PrimitiveMesh {
    Cube,       // 1 x 1 x 1, centered
    Sphere,     // diameter 1
    Cylinder,   // diameter 1, height 1 along Y, centered
    Plane,      // 1 x 1 on XZ, facing +Y
    Count
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

// Unit primitive drawn with a world matrix (renderer size is baked into the matrix).
struct RenderCommand3D {
    PrimitiveMesh mesh = PrimitiveMesh::Cube;
    Matrix transform = {};
    Color color = WHITE;
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

    void ConfigureTargetResolution(int width, int height);
    void Initialize();
    void Shutdown();
    void RenderFrame();

    void SetTargetResolution(int width, int height);
    Vector2 GetTargetResolution() const;
    Rectangle GetViewportRect() const;
    float GetRenderScale() const;
    Vector2 ScreenToTarget(Vector2 screenPosition) const;

    void Submit(const RenderCommand& cmd);
    void Submit(const RenderCommand3D& cmd);

    void Clear() {
        world2DCommands.clear();
        world3DCommands.clear();
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

    void SetClearColor(Color color) { clearColor = color; }
    // Direction the scene's single directional light travels in (world space).
    void SetLightDirection(Vector3 direction);

private:
    static constexpr std::size_t PrimitiveCount = static_cast<std::size_t>(PrimitiveMesh::Count);

    std::vector<RenderCommand> world2DCommands;
    std::vector<RenderCommand3D> world3DCommands;
    std::vector<RenderCommand> uiCommands;
    std::vector<Renderer*> world2DRenderers;
    std::vector<Renderer*> world3DRenderers;
    std::vector<Renderer*> uiRenderers;
    std::vector<CameraComponent*> registeredCameras;
    std::vector<RenderCommand>* activeCommandQueue = nullptr;
    
    Font defaultFont = {}; // 글로벌 기본 폰트 객체 보관용
    RenderTexture2D targetTexture = {};
    int targetWidth = 1920;
    int targetHeight = 1080;
    bool initialized = false;
    Color clearColor = BLACK;

    std::array<Mesh, PrimitiveCount> primitiveMeshes = {};
    std::array<Matrix, PrimitiveCount> primitivePivots = {};
    Material litMaterial = {};
    int lightDirectionLoc = -1;
    Vector3 lightDirection = { -0.35f, -0.85f, -0.4f };

    RenderSystem() = default;
    ~RenderSystem() = default;
    
    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;

    std::vector<Renderer*>* BucketOf(RenderBucket bucket);

    void CollectCommands(const std::vector<Renderer*>& renderers, std::vector<RenderCommand>* commands);
    void ExecuteCommands(const std::vector<RenderCommand>& cmds);
    void Execute3DCommands();
    void RecreateTargetTexture();
    void Load3DResources();
    void Unload3DResources();
};

}
