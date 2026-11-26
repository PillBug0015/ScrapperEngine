#include "Scrapper/Core/App.h"

#include <algorithm>
#include <chrono>
#include <cstdio>


namespace Scrapper {
App::App() : world(std::make_unique<World>()) {}

App::~App() {
    for (auto it = modules.rbegin(); it != modules.rend(); ++it) {
        (*it)->Uninstall(*world);
    }
    modules.clear();
    world.reset();
}

void App::Run() {
    std::fprintf(stderr, "[Pipeline]");
    for (const std::string& name : world->GetPipeline().Describe()) {
        std::fprintf(stderr, " > %s", name.c_str());
    }
    std::fprintf(stderr, "\n");

    using Clock = std::chrono::steady_clock;
    Clock::time_point last = Clock::now();
    while (!world->IsQuitRequested()) {
        const Clock::time_point now = Clock::now();
        const float deltaTime = std::min(std::chrono::duration<float>(now - last).count(), 0.25f);
        last = now;
        world->Step(deltaTime);
    }
}

}
