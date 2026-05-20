#include "Mob.h"
#include "Item.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    Mob* platoon[4];

    for(int i = 0; i < 4; i++)
    {
        platoon[i] = new Warrior(20, 10, "기본 전사");
        platoon[i]->Speak();
    }

    delete platoon;


    Mob* platoon2[4];

    for(int i = 0; i < 4; i++)
    {
        platoon2[i] = new Orc(20, 10, "오크");
        Mob* target = platoon2[0];
        platoon2[i]->Attack(target);
    }

    delete platoon2;
    return 0;
}