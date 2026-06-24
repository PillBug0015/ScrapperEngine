#pragma once

#include "Controllers/Controller.h"

namespace mud {
    class StoryController : public Controller
    {
    public:
        bool IsActive() const override;
        int GetPriority() const override;
        void Update(const UpdateContext& context) override;
    };
}
