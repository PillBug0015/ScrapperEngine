#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct DataHeader {
    std::string ID;
    std::string DisplayName;
};

enum class ModifierType {
    Buff,
    Debuff
};

struct ModifierData {
    DataHeader Header;
    ModifierType Type = ModifierType::Buff;
};

enum class StatType {
    Strength,
    Intelligence,
    Mobility
};

struct EntityData {
    DataHeader Header;

    int InitialMaxHealthPoint = 0;
    int InitialHealthPoint = 0;

    int InitialStrength = 0;
    int InitialIntelligence = 0;
    int InitialMobility = 0;

    std::vector<std::string> ActionIDs;
};

#pragma region Action
enum class TargetSide {
    Self,
    Enemy,
    Ally
};

enum class TargetMode {
    Single,
    AllSide,
    All
};

struct SelectTargetCommand {
    TargetSide Side = TargetSide::Enemy;
    TargetMode Mode = TargetMode::Single;
};

struct DealDamageCommand {
    int FlatAmount = 0;
    StatType StatKey = StatType::Strength;
    float Scale = 1.0f;
};

struct HealCommand {
    int Amount = 0;
};

struct AddModifierCommand {
    std::string ModifierID;
};

struct RemoveModifierCommand {
    std::string ModifierID;
};

struct PrintLogCommand {
    std::string Text;
};

using ActionCommandPayload = std::variant<
    SelectTargetCommand,
    DealDamageCommand,
    HealCommand,
    AddModifierCommand,
    RemoveModifierCommand,
    PrintLogCommand
>;

struct ActionCommand {
    ActionCommandPayload Payload;
};

struct ActionData {
    DataHeader Header;
    int APCost = 0;

    std::vector<ActionCommand> Commands;
};
#pragma endregion

struct ItemData {
    DataHeader Header;
    int Price = 0;
};

class GameDatabase {
private:
    std::unordered_map<std::string, EntityData> entities;
    std::unordered_map<std::string, ActionData> actions;
    std::unordered_map<std::string, ItemData> items;
    std::unordered_map<std::string, ModifierData> modifiers;

public:
    GameDatabase() = default;
    ~GameDatabase() = default;

    void Clear();

    bool LoadFile(const std::filesystem::path& path);
    int LoadDirectory(const std::filesystem::path& directory);

    void AddEntity(const EntityData& data);
    void AddAction(const ActionData& data);
    void AddItem(const ItemData& data);
    void AddModifier(const ModifierData& data);

    const EntityData* FindEntity(const std::string& id) const;
    const ActionData* FindAction(const std::string& id) const;
    const ItemData* FindItem(const std::string& id) const;
    const ModifierData* FindModifier(const std::string& id) const;

    int GetEntityCount() const;
    int GetActionCount() const;
    int GetItemCount() const;
    int GetModifierCount() const;
};
