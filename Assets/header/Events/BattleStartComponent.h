#pragma once

#include "Events/EventComponent.h"

#include <string>
#include <vector>

namespace mud {
    class BattleStartComponent : public EventComponent
    {
    public:
        std::vector<std::string> PlayerEntityIDs;
        std::vector<std::string> EnemyEntityIDs;

        void Execute() override;
    };
}
