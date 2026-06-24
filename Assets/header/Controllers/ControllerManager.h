#pragma once

#include "Controllers/Controller.h"

#include <vector>

namespace mud {
    class ControllerManager
    {
    private:
        std::vector<Controller*> controllers;
        std::vector<Controller*> modalControllers;

        Controller* FindActiveController() const;

    public:
        void Register(Controller& controller);

        void PushModal(Controller& controller);
        void PopModal(Controller& controller);

        void Update(const UpdateContext& context) const;
    };
}
