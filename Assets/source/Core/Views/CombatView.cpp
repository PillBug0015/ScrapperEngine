#include "Views/CombatView.h"

#include <algorithm>
#include <string>

namespace mud {
    namespace {
        constexpr Color BackgroundColor = { 12, 13, 18, 255 };
        constexpr Color PanelColor = { 24, 26, 34, 240 };
        constexpr Color PanelDarkColor = { 15, 16, 22, 245 };
        constexpr Color BorderColor = { 235, 236, 226, 255 };
        constexpr Color SoftBorderColor = { 94, 98, 118, 255 };
        constexpr Color TextColor = { 246, 244, 229, 255 };
        constexpr Color MutedTextColor = { 171, 175, 190, 255 };
        constexpr Color AccentColor = { 255, 221, 95, 255 };
        constexpr Color EnemyColor = { 88, 71, 112, 255 };
        constexpr Color PlayerColor = { 43, 82, 91, 255 };
        constexpr Color HpBackColor = { 50, 52, 61, 255 };
        constexpr Color HpFillColor = { 226, 82, 87, 255 };
        constexpr Color CommandHighlightColor = { 64, 57, 34, 255 };

        constexpr float ScreenWidth = 960.0f;
        constexpr float ScreenHeight = 540.0f;
        constexpr float BorderThickness = 2.0f;
        constexpr float SlotGap = 14.0f;

        std::string MakeHpText(const BattleEntityRenderData& entity)
        {
            return "HP " + std::to_string(std::max(0, entity.Hp)) + "/" + std::to_string(std::max(1, entity.MaxHp));
        }

        float GetHpRatio(const BattleEntityRenderData& entity)
        {
            const int maxHp = std::max(1, entity.MaxHp);
            const int hp = std::clamp(entity.Hp, 0, maxHp);
            return static_cast<float>(hp) / static_cast<float>(maxHp);
        }
    }

    void CombatView::Start()
    {
        BuildView();
        SetViewVisible(false);
    }

    void CombatView::OnEnable()
    {
        SetViewVisible(true);
        dirty = true;
    }

    void CombatView::OnDisable()
    {
        SetViewVisible(false);
    }

    void CombatView::Update(float /*deltaTime*/)
    {
        if (dirty)
        {
            ApplyData();
            dirty = false;
        }
    }

    void CombatView::SetData(const BattleRenderData& newData)
    {
        data = newData;
        dirty = true;
    }

    void CombatView::ClearData()
    {
        data = BattleRenderData();
        dirty = true;
    }

    void CombatView::BuildView()
    {
        AddRect({ ScreenWidth * 0.5f, ScreenHeight * 0.5f }, { ScreenWidth, ScreenHeight }, BackgroundColor, 0);

        AddPanel({ 18.0f, 18.0f }, { 924.0f, 126.0f }, PanelDarkColor, BorderColor, 1);
        AddPanel({ 76.0f, 164.0f }, { 808.0f, 84.0f }, PanelColor, SoftBorderColor, 1);
        AddPanel({ 18.0f, 270.0f }, { 924.0f, 130.0f }, PanelDarkColor, BorderColor, 1);
        AddPanel({ 18.0f, 418.0f }, { 924.0f, 104.0f }, PanelColor, BorderColor, 1);

        enemySlots.reserve(MaxEnemies);
        for (int i = 0; i < MaxEnemies; i++)
        {
            enemySlots.push_back(AddEntitySlot(3));
        }

        playerSlots.reserve(MaxPlayers);
        for (int i = 0; i < MaxPlayers; i++)
        {
            playerSlots.push_back(AddEntitySlot(3));
        }

        logLines.reserve(MaxLogLines);
        for (int i = 0; i < MaxLogLines; i++)
        {
            logLines.push_back(AddText({ 104.0f, 184.0f + static_cast<float>(i) * 30.0f }, "", 22, TextColor, 4));
        }

        commandSlots.reserve(MaxCommands);
        for (int i = 0; i < MaxCommands; i++)
        {
            CommandSlot slot;
            slot.highlight = AddRect({ 0.0f, 0.0f }, { 0.0f, 0.0f }, CommandHighlightColor, 2, { 0.0f, 0.0f });
            slot.cursor = AddText({ 0.0f, 0.0f }, "", 22, AccentColor, 4);
            slot.label = AddText({ 0.0f, 0.0f }, "", 22, TextColor, 4);
            commandSlots.push_back(slot);
        }
    }

    void CombatView::ApplyData()
    {
        const float enemySlotWidth = (884.0f - SlotGap * static_cast<float>(MaxEnemies - 1)) / static_cast<float>(MaxEnemies);
        const float playerSlotWidth = (884.0f - SlotGap * static_cast<float>(MaxPlayers - 1)) / static_cast<float>(MaxPlayers);

        for (int i = 0; i < MaxEnemies; i++)
        {
            const BattleEntityRenderData* entity = i < static_cast<int>(data.Enemies.size()) ? &data.Enemies[i] : nullptr;
            const Vector2 topLeft = { 38.0f + static_cast<float>(i) * (enemySlotWidth + SlotGap), 38.0f };
            ApplyEntitySlot(enemySlots[i], entity, topLeft, { enemySlotWidth, 86.0f }, true);
        }

        for (int i = 0; i < MaxPlayers; i++)
        {
            const BattleEntityRenderData* entity = i < static_cast<int>(data.Players.size()) ? &data.Players[i] : nullptr;
            const Vector2 topLeft = { 38.0f + static_cast<float>(i) * (playerSlotWidth + SlotGap), 292.0f };
            ApplyEntitySlot(playerSlots[i], entity, topLeft, { playerSlotWidth, 82.0f }, false);
        }

        const int logCount = std::min(MaxLogLines, static_cast<int>(data.LogLines.size()));
        const int logStart = std::max(0, static_cast<int>(data.LogLines.size()) - logCount);
        for (int i = 0; i < MaxLogLines; i++)
        {
            const bool visible = i < logCount;
            logLines[i]->SetVisible(visible);
            logLines[i]->text = visible ? data.LogLines[logStart + i] : "";
        }

        for (int i = 0; i < MaxCommands; i++)
        {
            const std::string* command = i < static_cast<int>(data.Commands.size()) ? &data.Commands[i] : nullptr;
            const int column = i % 4;
            const int row = i / 4;
            const Vector2 position = { 62.0f + static_cast<float>(column) * 218.0f, 442.0f + static_cast<float>(row) * 36.0f };
            ApplyCommandSlot(commandSlots[i], command, i == data.SelectedCommandIndex, position);
        }
    }

    void CombatView::SetViewVisible(bool visible)
    {
        for (::Renderer* renderer : renderers)
        {
            if (renderer != nullptr)
            {
                renderer->SetVisible(visible);
            }
        }
    }

    RectRenderer* CombatView::AddRect(Vector2 position, Vector2 size, Color color, int layer, Vector2 pivot)
    {
        RectRenderer* rect = owner->AddComponent<RectRenderer>(size, color, layer);
        rect->offset = position;
        rect->pivot = pivot;
        renderers.push_back(rect);
        return rect;
    }

    TextRenderer* CombatView::AddText(Vector2 position, const std::string& text, int fontSize, Color color, int layer)
    {
        TextRenderer* textRenderer = owner->AddComponent<TextRenderer>(text, fontSize, color, layer);
        textRenderer->offset = position;
        renderers.push_back(textRenderer);
        return textRenderer;
    }

    void CombatView::AddPanel(Vector2 topLeft, Vector2 size, Color fillColor, Color borderColor, int layer)
    {
        AddRect({ topLeft.x, topLeft.y }, size, fillColor, layer, { 0.0f, 0.0f });
        AddRect({ topLeft.x, topLeft.y }, { size.x, BorderThickness }, borderColor, layer + 1, { 0.0f, 0.0f });
        AddRect({ topLeft.x, topLeft.y + size.y - BorderThickness }, { size.x, BorderThickness }, borderColor, layer + 1, { 0.0f, 0.0f });
        AddRect({ topLeft.x, topLeft.y }, { BorderThickness, size.y }, borderColor, layer + 1, { 0.0f, 0.0f });
        AddRect({ topLeft.x + size.x - BorderThickness, topLeft.y }, { BorderThickness, size.y }, borderColor, layer + 1, { 0.0f, 0.0f });
    }

    CombatView::EntitySlot CombatView::AddEntitySlot(int layer)
    {
        EntitySlot slot;
        slot.body = AddRect({ 0.0f, 0.0f }, { 0.0f, 0.0f }, PlayerColor, layer, { 0.0f, 0.0f });
        slot.hpBack = AddRect({ 0.0f, 0.0f }, { 0.0f, 0.0f }, HpBackColor, layer + 1, { 0.0f, 0.0f });
        slot.hpFill = AddRect({ 0.0f, 0.0f }, { 0.0f, 0.0f }, HpFillColor, layer + 2, { 0.0f, 0.0f });
        slot.name = AddText({ 0.0f, 0.0f }, "", 20, TextColor, layer + 3);
        slot.hpText = AddText({ 0.0f, 0.0f }, "", 18, MutedTextColor, layer + 3);
        return slot;
    }

    void CombatView::ApplyEntitySlot(EntitySlot& slot, const BattleEntityRenderData* entity, Vector2 topLeft, Vector2 size, bool enemySide)
    {
        const bool visible = entity != nullptr;
        slot.body->SetVisible(visible);
        slot.hpBack->SetVisible(visible);
        slot.hpFill->SetVisible(visible);
        slot.name->SetVisible(visible);
        slot.hpText->SetVisible(visible);

        if (!visible)
        {
            return;
        }

        slot.body->offset = topLeft;
        slot.body->size = size;
        slot.body->color = enemySide ? EnemyColor : PlayerColor;

        slot.name->offset = { topLeft.x + 14.0f, topLeft.y + 12.0f };
        slot.name->text = entity->Name;

        const float hpWidth = size.x - 28.0f;
        const float hpY = topLeft.y + size.y - 30.0f;
        slot.hpBack->offset = { topLeft.x + 14.0f, hpY };
        slot.hpBack->size = { hpWidth, 10.0f };

        slot.hpFill->offset = { topLeft.x + 14.0f, hpY };
        slot.hpFill->size = { hpWidth * GetHpRatio(*entity), 10.0f };

        slot.hpText->offset = { topLeft.x + 14.0f, hpY + 16.0f };
        slot.hpText->text = MakeHpText(*entity);
    }

    void CombatView::ApplyCommandSlot(CommandSlot& slot, const std::string* command, bool selected, Vector2 position)
    {
        const bool visible = command != nullptr;
        slot.highlight->SetVisible(visible && selected);
        slot.cursor->SetVisible(visible);
        slot.label->SetVisible(visible);

        if (!visible)
        {
            return;
        }

        slot.highlight->offset = { position.x - 8.0f, position.y - 5.0f };
        slot.highlight->size = { 186.0f, 32.0f };
        slot.cursor->offset = position;
        slot.cursor->text = selected ? ">" : "";
        slot.label->offset = { position.x + 30.0f, position.y };
        slot.label->text = *command;
        slot.label->color = selected ? AccentColor : TextColor;
    }
}
