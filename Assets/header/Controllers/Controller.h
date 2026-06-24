#pragma once

#include "Controllers/UpdateContext.h"

namespace mud {
    class Controller
    {
    public:
        virtual ~Controller() = default;

        virtual bool IsActive() const = 0;
        virtual int GetPriority() const = 0;
        virtual void Update(const UpdateContext& context) = 0;
    };
}
