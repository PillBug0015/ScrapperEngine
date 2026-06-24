#pragma once

#include "Events/ChoiceComponent.h"

#include <string>
#include <vector>

namespace mud {
    class ChoiceSystem
    {
    private:
        std::vector<ChoiceData> choices;
        int selectedIndex = 0;
        bool open = false;

        ChoiceSystem() = default;

    public:
        static ChoiceSystem& Instance();

        void Open(const std::vector<ChoiceData>& newChoices);
        void Close();

        void MoveUp();
        void MoveDown();
        std::string Confirm();

        bool IsOpen() const;
        int GetSelectedIndex() const;
        std::vector<std::string> GetChoiceLabels() const;
    };
}
