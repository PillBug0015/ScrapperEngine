#include "EventEngine.h"

namespace mud {
    
    EventEngine& EventEngine::Instance()
    {
        static EventEngine instance;  
        return instance;
    }

    void EventEngine::Process()
    {
        Event& currentEvent = *Events[currentEventsIndex];
    
        for (const std::unique_ptr<EventComponent>& component : currentEvent.Components)
        {
            if (component != nullptr) 
            {
                component->Execute(); 
            }
        }

        currentEventsIndex++;
    }
}