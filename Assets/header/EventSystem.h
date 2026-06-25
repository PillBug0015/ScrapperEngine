#pragma once

#include "EventComponents.h"
#include <vector>
#include <memory>

class GameManager;

class EventSystem {
public:
    EventSystem() = default;
    ~EventSystem() = default;

    // Prevent copying
    EventSystem(const EventSystem&) = delete;
    EventSystem& operator=(const EventSystem&) = delete;

    void QueueEvent(std::unique_ptr<EventComponent> event) {
        eventQueue.push_back(std::move(event));
    }

    void Update(float deltaTime, GameManager* game);
    void Draw(GameManager* game);

    bool IsPlaying() const {
        return activeEvent != nullptr || !eventQueue.empty();
    }

    EventComponent* GetActiveEvent() const {
        return activeEvent;
    }

    void ClearQueue() {
        eventQueue.clear();
        activeEvent = nullptr;
    }

private:
    std::vector<std::unique_ptr<EventComponent>> eventQueue;
    EventComponent* activeEvent = nullptr;

    void NextEvent(GameManager* game);
};
