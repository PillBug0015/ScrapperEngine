#pragma once

#include "Scrapper/Core/Module.h"
#include "Scrapper/Core/World.h"

#include <memory>
#include <utility>
#include <vector>


namespace Scrapper {
// Owns a World and the modules installed into it, and steps the pipeline until someone requests quit.
class App {
public:
    App();
    ~App(); // uninstalls modules in reverse order

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    template <typename M, typename... Args>
    M& Install(Args&&... args) {
        auto module = std::make_unique<M>(std::forward<Args>(args)...);
        M& ref = *module;
        ref.Install(*world);
        modules.push_back(std::move(module));
        return ref;
    }

    World& GetWorld() { return *world; }
    void Run();

private:
    std::unique_ptr<World> world;
    std::vector<std::unique_ptr<Module>> modules;
};

}
