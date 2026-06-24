#include "Systems/ChoiceSystem.h"

#include <algorithm>

namespace mud {
    ChoiceSystem& ChoiceSystem::Instance()
    {
        static ChoiceSystem instance;
        return instance;
    }

    void ChoiceSystem::Open(const std::vector<ChoiceData>& newChoices)
    {
        choices = newChoices;
        selectedIndex = 0;
        open = !choices.empty();
    }

    void ChoiceSystem::Close()
    {
        choices.clear();
        selectedIndex = 0;
        open = false;
    }

    void ChoiceSystem::MoveUp()
    {
        if (!open || choices.empty())
        {
            return;
        }

        selectedIndex = std::max(0, selectedIndex - 1);
    }

    void ChoiceSystem::MoveDown()
    {
        if (!open || choices.empty())
        {
            return;
        }

        selectedIndex = std::min(static_cast<int>(choices.size()) - 1, selectedIndex + 1);
    }

    std::string ChoiceSystem::Confirm()
    {
        if (!open || choices.empty())
        {
            return "";
        }

        const std::string targetEventID = choices[selectedIndex].TargetEventID;
        Close();
        return targetEventID;
    }

    bool ChoiceSystem::IsOpen() const
    {
        return open;
    }

    int ChoiceSystem::GetSelectedIndex() const
    {
        return selectedIndex;
    }

    std::vector<std::string> ChoiceSystem::GetChoiceLabels() const
    {
        std::vector<std::string> labels;
        labels.reserve(choices.size());

        for (const ChoiceData& choice : choices)
        {
            labels.push_back(choice.DisplayName);
        }

        return labels;
    }
}
