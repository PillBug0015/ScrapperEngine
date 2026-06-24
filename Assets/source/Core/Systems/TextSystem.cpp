#include "Systems/TextSystem.h"

namespace mud {
    TextSystem& TextSystem::Instance()
    {
        static TextSystem instance;
        return instance;
    }

    void TextSystem::AddText(const std::string& text)
    {
        lines.push_back(text);
    }

    void TextSystem::Clear()
    {
        lines.clear();
    }

    const std::vector<std::string>& TextSystem::GetLines() const
    {
        return lines;
    }
}
