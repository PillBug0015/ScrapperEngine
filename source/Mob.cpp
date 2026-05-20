#include "Mob.h"
#include <iostream>


#pragma region Mob
Mob::Mob(int h, int a, std::string n) : hp(h), maxHP(h), atk(a), def(0), name(n), experience(0), maxEx(10), level(1)
{
    std::cout << name << "가 소환되었다!" << std::endl;
}

Mob::~Mob()
{
    std::cout << name << "가 퇴장합니다." << std::endl;
}

void Mob::Attack(Mob* target)
{
    std::cout << name << "가 공격한다!" << std::endl;
    target->GetDamage(atk);
}

void Mob::Heal(int amount)
{
    hp += amount;
    if(hp > maxHP){
        hp = maxHP;
    }
    std::cout << name << "가 치료되었다!  현재 체력:" << hp << std::endl;
}

void Mob::GetDamage(int amount)
{
    amount = amount - def;
    if (amount < 0)
    {
        amount = 0;
    }

    hp -= amount;
    if (hp < 0)
    {
        hp = 0;
    }

    std::cout << name << "가 맞았다!  현재 체력:" << hp << std::endl;
    if (!IsAlive())
    {
        Dead();
    }
}

bool Mob::IsAlive() const
{
    return hp > 0;
}

void Mob::Dead()
{
    std::cout << name << "가 죽었다~" << std::endl;
}

void Mob::ShowState() const
{
    std::cout << "===" << name << "의 상태창 ===" << std::endl;
    std::cout << "hp:" << hp << "/" << maxHP << std::endl;


    std::cout << "level:" << level << " exp:" << experience << "/" << maxEx << std::endl;
    std::cout << "atk:" << atk << std::endl;
    std::cout << "def:" << def << std::endl;  
    std::cout << "=== === === === === ===" << std::endl;
}

void Mob::Speak()
{
}

void Mob::GainExp(int amount)
{
    experience += amount;
    std::cout << name << "가 경험치를 얻었다!" << std::endl;

    while(experience >= maxEx)
    {
        experience -= maxEx;
        LevelUp();
    }
}

void Mob::LevelUp()
{
    level++;
    maxHP += 5;
    hp = maxHP;
    atk += 1;
    def += 1;
    maxEx += 10;
    std::cout << name << "가 레벨 업 했다!" << std::endl;
}
#pragma endregion