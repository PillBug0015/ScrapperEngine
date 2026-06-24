#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct DataHeader {
    std::string ID;
    std::string DisplayName;
};

#pragma region modifier
enum class ModifierType {
    Buff,
    Debuff
};

struct ModifierData {
    DataHeader Header;
    ModifierType Type = ModifierType::Buff;
};
#pragma endregion

#pragma region Action
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
#pragma endregion

#pragma region ActionCommands
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

#pragma region EventCommands
struct TextCommand {
    std::string Text;
};

struct ChoiceContainer {
    std::string Text;
    std::string TargetEvent;
};

struct ChoiceCommand {
    std::vector<ChoiceContainer> Choices;
};

struct CombatCommand {
    std::vector<std::string> EnemiesID;
};

using EventCommandPayload = std::variant<
    TextCommand,
    ChoiceCommand,
    CombatCommand
>;

struct EventCommand {
    EventCommandPayload Payload;
};

struct EventData {
    DataHeader Header;

    std::vector<EventCommand> Commands;
};
#pragma endregion

struct ItemData {
    DataHeader Header;
    int Price = 0;
};

#pragma region DB
class GameDatabase {
private:
    std::unordered_map<std::string, EntityData> entities;
    std::unordered_map<std::string, ActionData> actions;
    std::unordered_map<std::string, ItemData> items;
    std::unordered_map<std::string, ModifierData> modifiers;
    std::unordered_map<std::string, EventData> events; // 추가된 이벤트 보관소

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
    void AddEvent(const EventData& data); // 추가

    const EntityData* FindEntity(const std::string& id) const;
    const ActionData* FindAction(const std::string& id) const;
    const ItemData* FindItem(const std::string& id) const;
    const ModifierData* FindModifier(const std::string& id) const;
    const EventData* FindEvent(const std::string& id) const; // 추가

    int GetEntityCount() const;
    int GetActionCount() const;
    int GetItemCount() const;
    int GetModifierCount() const;
    int GetEventCount() const; // 추가
};
#pragma endregion

#pragma region Json helpers
inline std::string ToLowerHelper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline void from_json(const json& j, StatType& stat) {
    std::string value = ToLowerHelper(j.get<std::string>());
    if (value == "intelligence" || value == "int") stat = StatType::Intelligence;
    else if (value == "mobility" || value == "mob") stat = StatType::Mobility;
    else stat = StatType::Strength;
}

inline void from_json(const json& j, TargetSide& side) {
    std::string value = ToLowerHelper(j.get<std::string>());
    if (value == "self") side = TargetSide::Self;
    else if (value == "ally" || value == "allies") side = TargetSide::Ally;
    else side = TargetSide::Enemy;
}

inline void from_json(const json& j, TargetMode& mode) {
    std::string value = ToLowerHelper(j.get<std::string>());
    if (value == "allside" || value == "side") mode = TargetMode::AllSide;
    else if (value == "all") mode = TargetMode::All;
    else mode = TargetMode::Single;
}

inline void from_json(const json& j, ModifierType& type) {
    type = (ToLowerHelper(j.get<std::string>()) == "debuff") ? ModifierType::Debuff : ModifierType::Buff;
}

inline void from_json(const json& j, DataHeader& header) {
    header.ID = j.value("id", j.value("uniqueID", ""));
    header.DisplayName = j.value("displayName", j.value("name", j.value("displayInitialName", "이름 없음")));
}

inline void from_json(const json& j, ModifierData& data) {
    from_json(j, data.Header);
    data.Type = j.value("modifierType", j.value("type", ModifierType::Buff));
}

inline void from_json(const json& j, EntityData& data) {
    from_json(j, data.Header);
    const json& stats = j.contains("stats") && j["stats"].is_object() ? j["stats"] : j;
    data.InitialMaxHealthPoint = stats.value("maxHealthPoint", stats.value("maxHp", stats.value("maxHP", 0)));
    data.InitialHealthPoint = stats.value("healthPoint", stats.value("hp", stats.value("HP", data.InitialMaxHealthPoint)));
    data.InitialStrength = stats.value("strength", stats.value("str", 0));
    data.InitialIntelligence = stats.value("intelligence", stats.value("int", 0));
    data.InitialMobility = stats.value("mobility", stats.value("mob", 0));
    data.ActionIDs = j.value("actionIds", j.value("actionIDs", std::vector<std::string>{}));
}

inline void from_json(const json& j, SelectTargetCommand& cmd) {
    cmd.Side = j.value("side", TargetSide::Enemy);
    cmd.Mode = j.value("mode", TargetMode::Single);
}

inline void from_json(const json& j, DealDamageCommand& cmd) {
    cmd.FlatAmount = j.value("flatAmount", j.value("amount", j.value("damage", 0)));
    cmd.StatKey = j.value("stat", j.value("statKey", StatType::Strength));
    cmd.Scale = j.value("scale", 1.0f);
}

inline void from_json(const json& j, HealCommand& cmd) {
    cmd.Amount = j.value("amount", j.value("heal", 0));
}

inline void from_json(const json& j, AddModifierCommand& cmd) {
    cmd.ModifierID = j.value("modifierId", j.value("modifierID", j.value("id", "")));
}

inline void from_json(const json& j, RemoveModifierCommand& cmd) {
    cmd.ModifierID = j.value("modifierId", j.value("modifierID", j.value("id", "")));
}

inline void from_json(const json& j, PrintLogCommand& cmd) {
    cmd.Text = j.value("text", j.value("log", ""));
}

inline void from_json(const json& j, ActionCommand& cmd) {
    std::string type = ToLowerHelper(j.value("type", j.value("commandType", "")));

    if (type == "selecttarget" || type == "target") {
        SelectTargetCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "dealdamage" || type == "damage") {
        DealDamageCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "heal") {
        HealCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "addmodifier") {
        AddModifierCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "removemodifier") {
        RemoveModifierCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else {
        PrintLogCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
}

inline void from_json(const json& j, ActionData& data) {
    from_json(j, data.Header);
    data.APCost = j.value("apCost", j.value("mpCost", 0));

    if (j.contains("commands") && j["commands"].is_array()) {
        data.Commands = j["commands"].get<std::vector<ActionCommand>>();
    }
    else if (j.contains("damage")) {
        SelectTargetCommand targetCmd;
        PrintLogCommand logCmd;
        logCmd.Text = "{user} uses {action}!";
        DealDamageCommand damageCmd;
        damageCmd.FlatAmount = j.value("damage", 0);
        damageCmd.StatKey = StatType::Strength;
        damageCmd.Scale = 1.0f;

        data.Commands.push_back({ targetCmd });
        data.Commands.push_back({ logCmd });
        data.Commands.push_back({ damageCmd });
    }
}

inline void from_json(const json& j, ItemData& data) {
    from_json(j, data.Header);
    data.Price = j.value("price", 0);
}

// -----------------------------------------------------------------
// 이벤트 관련 inline from_json 정의 추가
// -----------------------------------------------------------------

inline void from_json(const json& j, TextCommand& cmd) {
    cmd.Text = j.value("text", "");
}

inline void from_json(const json& j, ChoiceContainer& container) {
    container.Text = j.value("text", "");
    container.TargetEvent = j.value("targetEvent", j.value("target", ""));
}

inline void from_json(const json& j, ChoiceCommand& cmd) {
    if (j.contains("choices") && j["choices"].is_array()) {
        cmd.Choices = j["choices"].get<std::vector<ChoiceContainer>>();
    }
}

inline void from_json(const json& j, CombatCommand& cmd) {
    cmd.EnemiesID = j.value("enemies", j.value("enemiesID", j.value("enemiesId", std::vector<std::string>{})));
}

inline void from_json(const json& j, EventCommand& cmd) {
    std::string type = ToLowerHelper(j.value("type", j.value("commandType", "")));

    if (type == "text") {
        TextCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "choice" || type == "choices") {
        ChoiceCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
    else if (type == "combat" || type == "battle") {
        CombatCommand payload;
        from_json(j, payload);
        cmd.Payload = payload;
    }
}

inline void from_json(const json& j, EventData& data) {
    from_json(j, data.Header);
    if (j.contains("commands") && j["commands"].is_array()) {
        data.Commands = j["commands"].get<std::vector<EventCommand>>();
    }
}
#pragma endregion