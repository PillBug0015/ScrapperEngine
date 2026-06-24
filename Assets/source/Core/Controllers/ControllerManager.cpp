#include "Controllers/ControllerManager.h"

#include <algorithm>

namespace mud {
    Controller* ControllerManager::FindActiveController() const
    {
        Controller* activeController = nullptr;

        for (Controller* controller : controllers)
        {
            if (controller == nullptr || !controller->IsActive())
            {
                continue;
            }

            if (activeController == nullptr || controller->GetPriority() > activeController->GetPriority())
            {
                activeController = controller;
            }
        }

        return activeController;
    }

    void ControllerManager::Register(Controller& controller)
    {
        controllers.push_back(&controller);
    }

    void ControllerManager::PushModal(Controller& controller)
    {
        modalControllers.push_back(&controller);
    }

    void ControllerManager::PopModal(Controller& controller)
    {
        const auto foundController = std::find(modalControllers.begin(), modalControllers.end(), &controller);

        if (foundController != modalControllers.end())
        {
            modalControllers.erase(foundController);
        }
    }

    void ControllerManager::Update(const UpdateContext& context) const
    {
        if (!modalControllers.empty())
        {
            Controller* modalController = modalControllers.back();

            if (modalController != nullptr)
            {
                modalController->Update(context);
            }

            return;
        }

        Controller* activeController = FindActiveController();

        if (activeController != nullptr)
        {
            activeController->Update(context);
        }
    }
}
