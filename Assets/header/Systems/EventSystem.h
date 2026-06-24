#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Events/Scene.h"

namespace mud {
    class EventSystem
    {
    private:
        std::unique_ptr<Scene> currentScene;
        std::unordered_map<std::string, Event*> EventMap;

        int currentEventsIndex = 0;
        int currentComponentIndex = 0;

        EventSystem() = default; 
        void JumpTo(int eventIndex);

    public:
        static EventSystem& Instance();
        
        // public API
        void PlayNext();

        void JumpTo(const std::string& eventID);

        bool HasEvent() const;

        void LoadScene(std::unique_ptr<Scene> scene);
        void AddEvent(std::unique_ptr<Event> event);
        void Clear();
    };
}
