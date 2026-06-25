#include "System/EventSystem/EventSystem.h"

namespace MyMudGame {

void EventSystem::Initialize(const GameDatabase* gameDatabase, GameManager* manager)
{
    database = gameDatabase;
    gameManager = manager;
}

bool EventSystem::StartEvent(const std::string& eventId)
{
    if (database == nullptr || database->FindEvent(eventId) == nullptr)
    {
        eventState = EventState::Idle;
        return false;
    }

    eventState = EventState::Typing;
    return true;
}

void EventSystem::Continue()
{
    eventState = EventState::Idle;
}

void EventSystem::Update(float /*deltaTime*/)
{
}

void EventSystem::OnActivated()
{
}

void EventSystem::OnDeactivated()
{
    eventState = EventState::Idle;
}

void EventSystem::OnPaused()
{
}

void EventSystem::OnNavigateInput()
{
    (void)navigateInterval;
}

void EventSystem::OnCommitInput()
{
}

}
