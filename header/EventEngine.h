#pragma once

#include <string>
#include <vector>
#include <memory>


namespace mud {

    class Event
    {
    public:
        std::string UniqueID;
        int ManagedIndex;
        std::vector<std::unique_ptr<EventComponent>> Components;

        ~Event() = default;
    };
    
    class EventComponent {
    public:
        virtual void Execute() = 0;
    };

    class EventEngine
    {
    private:
        EventEngine() = default; 

    public:
        std::vector<std::unique_ptr<Event>> Events;
        int currentEventsIndex;

        static EventEngine& Instance();

        void Process();
    };
}