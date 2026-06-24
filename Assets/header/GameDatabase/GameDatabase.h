#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct DataHeader {
    std::string UniqueID;
    std::string DisplayInitialName;

    ~DataHeader() = default;
};


enum class ModifierCommandType {

};
enum class ModifierType{

};
struct ModifierData {
    DataHeader Header;
};



enum class StatType{
    Strength,
    Intelligence,
    Mobility
};
struct EntityData {
    DataHeader Header;

    int IntialMaxHealthPoint;
    int IntialHealthPoint;

    int IntialStrength;
    int IntialIntelligence;
    int IntialMobility;
};




enum class ActionCommandType {
    SelectTarget,
    DealDamage,   
    Heal,
    AddModifier, // buff, debuff
    RemoveModifier, // cure~
    PrintText // write log.
};
struct ActionData {
    DataHeader Header;
    int APCost;    
};




struct ItemData  {
    DataHeader Header;
};


class GameDatabase
{
private:
public:
    GameDatabase(/* args */);
    ~GameDatabase();
};

