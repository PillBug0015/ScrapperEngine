#pragma once

#include <string>
#include <vector>

namespace mud {

    class TextSystem
    {
    private:
        std::vector<std::string> lines;

        TextSystem() = default;

    public:
        static TextSystem& Instance();

        void AddText(const std::string& text);
        void Clear();

        const std::vector<std::string>& GetLines() const;
    };
}
