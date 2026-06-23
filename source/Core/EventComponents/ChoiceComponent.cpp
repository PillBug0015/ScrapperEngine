#include "EventComponents\ChoiceComponent.h"
#include <iostream>


namespace mud {
    void ChoiceComponent::Execute()
    {
        int choiceCount = Choices.size();

        for(int i = 0; i < choiceCount; i++)
        {
            std::cout << Choices[i].DisplayName << std::endl;
        }
        

        std::cout << " === 선택 하라. === " << std::endl;

        int selectedIndex;
        while(true)
        {
            std::cin >> selectedIndex;

            if(selectedIndex >= 0 && selectedIndex < choiceCount) break;
        }

        std::cout << " === 선택 끝. === " << std::endl;
    }
}
