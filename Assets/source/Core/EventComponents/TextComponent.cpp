#include "Events/TextComponent.h"
#include "Systems/TextSystem.h"

namespace mud {
    void TextComponent::Execute()
    {
        TextSystem::Instance().AddText(content);
    }
}
