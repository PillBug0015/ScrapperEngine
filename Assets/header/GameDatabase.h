#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace MyMudGame {

using json = nlohmann::json;

// 모든 정적 데이터의 공통 부모 구조체
struct BaseData {
    std::string uniqueID;
    std::string displayInitialName;

    virtual ~BaseData() = default;
};

// 생명체 정적 데이터 (NPC, 몬스터, 플레이어 원본)
struct EntityData : public BaseData {
    // 공간 이동이 없는 MUD 게임이므로 위치/기호 관련 변수가 없습니다.
};

// 아이템 정적 데이터
struct ItemData : public BaseData {
    int price = 0;
};

// 전투 행동 정적 데이터 (스킬, 마법 등)
struct ActionData : public BaseData {
    int mpCost = 0; // 마나 소모량
    int damage = 0; // 데미지 (음수면 힐)
};

// ==========================================
// JSON -> Struct 자동 매핑용 inline 함수 정의
// ==========================================

inline void from_json(const json& j, BaseData& data) {
    // "uniqueID", "type", "id" 순서로 매핑을 시도합니다.
    data.uniqueID = j.value("uniqueID", j.value("type", j.value("id", "")));
    data.displayInitialName = j.value("displayInitialName", j.value("name", "이름 없음"));
}

inline void from_json(const json& j, EntityData& data) {
    from_json(j, static_cast<BaseData&>(data));
}

inline void from_json(const json& j, ItemData& data) {
    from_json(j, static_cast<BaseData&>(data));
    data.price = j.value("price", 0);
}

inline void from_json(const json& j, ActionData& data) {
    from_json(j, static_cast<BaseData&>(data));
    // "mpCost"와 "cost" 모두 유연하게 매핑하도록 지원합니다.
    data.mpCost = j.value("mpCost", j.value("cost", 0));
    data.damage = j.value("damage", 0);
}

// ==========================================
// GameDatabase 클래스 정의
// ==========================================

class GameDatabase {
private:
    std::unordered_map<std::string, EntityData> entities;
    std::unordered_map<std::string, ItemData> items;
    std::unordered_map<std::string, ActionData> actions;

public:
    GameDatabase() = default;
    ~GameDatabase() = default;

    bool LoadDatabase(const std::string& directoryPath);

    const EntityData* GetEntityData(const std::string& id) const;
    const ItemData* GetItemData(const std::string& id) const;
    const ActionData* GetActionData(const std::string& id) const;

    size_t GetEntityCount() const { return entities.size(); }
    size_t GetItemCount() const { return items.size(); }
    size_t GetActionCount() const { return actions.size(); }
};

}
