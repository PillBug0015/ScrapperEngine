#pragma once

#include "Component.h"
#include "RectRenderer.h"
#include "Data/RenderData.h"
#include "TextRenderer.h"

#include <string>
#include <vector>

namespace mud {
    class CombatView : public Component
    {
    public:
        void Start() override;
        void OnEnable() override;
        void OnDisable() override;
        void Update(float deltaTime) override;

        void SetData(const BattleRenderData& newData);
        void ClearData();

    private:
        struct EntitySlot
        {
            RectRenderer* body = nullptr;
            RectRenderer* hpBack = nullptr;
            RectRenderer* hpFill = nullptr;
            TextRenderer* name = nullptr;
            TextRenderer* hpText = nullptr;
        };

        struct CommandSlot
        {
            RectRenderer* highlight = nullptr;
            TextRenderer* cursor = nullptr;
            TextRenderer* label = nullptr;
        };

        static constexpr int MaxEnemies = 4;
        static constexpr int MaxPlayers = 4;
        static constexpr int MaxLogLines = 2;
        static constexpr int MaxCommands = 8;

        BattleRenderData data;
        bool dirty = true;

        std::vector<::Renderer*> renderers;
        std::vector<EntitySlot> enemySlots;
        std::vector<EntitySlot> playerSlots;
        std::vector<TextRenderer*> logLines;
        std::vector<CommandSlot> commandSlots;

        void BuildView();
        void ApplyData();
        void SetViewVisible(bool visible);

        RectRenderer* AddRect(Vector2 position, Vector2 size, Color color, int layer, Vector2 pivot = { 0.5f, 0.5f });
        TextRenderer* AddText(Vector2 position, const std::string& text, int fontSize, Color color, int layer);
        void AddPanel(Vector2 topLeft, Vector2 size, Color fillColor, Color borderColor, int layer);

        EntitySlot AddEntitySlot(int layer);
        void ApplyEntitySlot(EntitySlot& slot, const BattleEntityRenderData* entity, Vector2 topLeft, Vector2 size, bool enemySide);
        void ApplyCommandSlot(CommandSlot& slot, const std::string* command, bool selected, Vector2 position);
    };
}
