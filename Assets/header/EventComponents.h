#pragma once

#include <string>
#include <vector>
#include <functional>

class GameManager;

class EventComponent {
public:
    virtual ~EventComponent() = default;
    virtual void OnStart(GameManager* game) { (void)game; }
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
    virtual bool IsFinished() const { return true; }
    virtual void OnDraw() {}
};

// --- Concrete Event: Text Narration / Dialogue ---
class TextEvent : public EventComponent {
public:
    std::string text;
    bool finished = false;

    TextEvent(const std::string& text) : text(text) {}

    void OnStart(GameManager* game) override;
    void OnUpdate(float deltaTime) override;
    bool IsFinished() const override { return finished; }
};

// --- Concrete Event: Branching Choices ---
struct ChoiceOption {
    std::string label;
    std::function<void(GameManager*)> callback;
};

class ChoiceEvent : public EventComponent {
public:
    std::string text;
    std::vector<ChoiceOption> options;
    bool finished = false;

    ChoiceEvent(const std::string& text, const std::vector<ChoiceOption>& options)
        : text(text), options(options) {}

    void OnStart(GameManager* game) override;
    bool IsFinished() const override { return finished; }
    
    void SelectOption(int index, GameManager* game) {
        if (index >= 0 && index < (int)options.size()) {
            if (options[index].callback) {
                options[index].callback(game);
            }
            finished = true;
        }
    }
};

// --- Concrete Event: Combat Trigger ---
class CombatEvent : public EventComponent {
public:
    std::string enemyName;
    int enemyHp;
    int enemyAtk;

    CombatEvent(const std::string& name, int hp, int atk)
        : enemyName(name), enemyHp(hp), enemyAtk(atk) {}

    void OnStart(GameManager* game) override;
    bool IsFinished() const override;
};

// --- Concrete Event: Stat Reward / Penalty ---
class RewardEvent : public EventComponent {
public:
    std::string statKey;
    int delta;

    RewardEvent(const std::string& key, int delta) : statKey(key), delta(delta) {}

    void OnStart(GameManager* game) override;
    bool IsFinished() const override { return true; } // Completes instantly
};
