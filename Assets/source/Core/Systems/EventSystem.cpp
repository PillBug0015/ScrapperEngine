#include "Systems/EventSystem.h"

namespace mud {
    
    EventSystem& EventSystem::Instance()
    {
        static EventSystem instance;  
        return instance;
    }

    void EventSystem::PlayNext()
    {
        while (HasEvent())
        {
            Event& currentEvent = *currentScene->Events[currentEventsIndex];

            if (currentComponentIndex < static_cast<int>(currentEvent.Components.size()))
            {
                const std::unique_ptr<EventComponent>& component = currentEvent.Components[currentComponentIndex];
                currentComponentIndex++;

                if (component != nullptr) 
                {
                    component->Execute();
                }

                return;
            }

            currentEventsIndex++;
            currentComponentIndex = 0;
        }
    }

    void EventSystem::JumpTo(int eventIndex)
    {
        const int eventCount = currentScene == nullptr ? 0 : static_cast<int>(currentScene->Events.size());

        if (eventIndex < 0 || eventIndex >= eventCount)
        {
            currentEventsIndex = eventCount;
            currentComponentIndex = 0;
            return;
        }

        currentEventsIndex = eventIndex;
        currentComponentIndex = 0;
    }

    void EventSystem::JumpTo(const std::string& eventID)
    {
        const auto foundEvent = EventMap.find(eventID);

        if (foundEvent == EventMap.end() || foundEvent->second == nullptr)
        {
            const int eventCount = currentScene == nullptr ? 0 : static_cast<int>(currentScene->Events.size());
            currentEventsIndex = eventCount;
            currentComponentIndex = 0;
            return;
        }

        JumpTo(foundEvent->second->ManagedIndex);
    }

    bool EventSystem::HasEvent() const
    {
        return currentScene != nullptr &&
               currentEventsIndex >= 0 &&
               currentEventsIndex < static_cast<int>(currentScene->Events.size());
    }

    void EventSystem::LoadScene(std::unique_ptr<Scene> scene)
    {
        Clear();
        currentScene = std::move(scene);

        if (currentScene == nullptr)
        {
            return;
        }

        for (int i = 0; i < static_cast<int>(currentScene->Events.size()); i++)
        {
            Event* event = currentScene->Events[i].get();

            if (event != nullptr)
            {
                event->ManagedIndex = i;
                EventMap[event->UniqueID] = event;
            }
        }

        if (!currentScene->StartEventID.empty())
        {
            JumpTo(currentScene->StartEventID);
            return;
        }

        JumpTo(0);
    }

    void EventSystem::AddEvent(std::unique_ptr<Event> event)
    {
        if (event == nullptr)
        {
            return;
        }

        if (currentScene == nullptr)
        {
            currentScene = std::make_unique<Scene>();
        }

        event->ManagedIndex = static_cast<int>(currentScene->Events.size());
        EventMap[event->UniqueID] = event.get();
        currentScene->Events.push_back(std::move(event));
    }

    void EventSystem::Clear()
    {
        currentScene.reset();
        EventMap.clear();
        currentEventsIndex = 0;
        currentComponentIndex = 0;
    }
}
