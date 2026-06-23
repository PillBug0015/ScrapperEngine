#include "EventComponents\TextComponent.h"
#include <iostream>

namespace mud {
    void TextComponent::Execute()
    {
        std::cout << content << std::endl;
    }
}
