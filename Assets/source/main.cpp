#include "Scrapper.h"
#include "GameDatabase/GameDatabase.h"
#include <iostream>

int main()
{
    GameDatabase database;
    const int loadedFiles = database.LoadDirectory("data");

    std::cout << "Loaded JSON files: " << loadedFiles << '\n';
    std::cout << "Entities: " << database.GetEntityCount() << '\n';
    std::cout << "Actions: " << database.GetActionCount() << '\n';
    std::cout << "Items: " << database.GetItemCount() << '\n';
    std::cout << "Modifiers: " << database.GetModifierCount() << '\n';

    const EntityData* goblin = database.FindEntity("goblin_boss");
    if (goblin != nullptr)
    {
        std::cout << "Found entity: " << goblin->Header.DisplayName << '\n';
    }
}
