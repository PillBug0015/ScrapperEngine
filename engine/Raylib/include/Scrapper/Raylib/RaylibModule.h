#pragma once

#include "Scrapper/Core/Math.h"
#include "Scrapper/Core/Module.h"

#include <string>


namespace Scrapper {
namespace RaylibSystem {
inline constexpr const char* Layout = "Raylib.Layout";
inline constexpr const char* Render = "Raylib.Render";
}

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "ScrapperEngine";
    int targetWidth = 1280; // render resolution, letterboxed into the window
    int targetHeight = 720;
    int targetFps = 60;
    std::string fontPath = "Fonts/default.ttf";
};

// Window, input polling and drawing of the Raylib components. Requests quit when the window closes.
class RaylibModule : public Module {
public:
    explicit RaylibModule(WindowConfig config = {}) : config(std::move(config)) {}

    void Install(World& world) override;
    void Uninstall(World& world) override;

private:
    WindowConfig config;
};

}
