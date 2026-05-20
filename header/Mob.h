#pragma once

#include <string>
class Mob
{
protected:
    int hp;
    int maxHP;

    int atk;
    int def;

    std::string name;
    int experience;
    int maxEx;
    int level;

public:
    Mob(int h, int a, std::string n);
    virtual ~Mob();

    void Attack(Mob* target);

    void Heal(int amount);
    void GetDamage(int amount);


    bool IsAlive() const;
    void Dead();
    void ShowState() const;

    void GainExp(int amount);

    virtual void LevelUp();
    virtual void Speak();
};

class Warrior : public Mob
{
public:
    Warrior(int h, int a, std::string n);

    void LevelUp() override;    
    void Speak() override;
};

class Ranger : public Mob
{
public:
    Ranger(int h, int a, std::string n);

    void LevelUp() override;    
    void Speak() override;
};

class Orc : public Mob
{
public:
    Orc(int h, int a, std::string n);

    void LevelUp() override;    
    void Speak() override;
};
