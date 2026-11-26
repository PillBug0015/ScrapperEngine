#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>


namespace Scrapper {
class World;

// Anchors every module can place its systems between, in this order.
namespace Phase {
inline constexpr const char* Input = "Phase.Input";
inline constexpr const char* Update = "Phase.Update";
inline constexpr const char* PreSimulate = "Phase.PreSimulate";
inline constexpr const char* Simulate = "Phase.Simulate";
inline constexpr const char* PostSimulate = "Phase.PostSimulate";
inline constexpr const char* LateUpdate = "Phase.LateUpdate";
inline constexpr const char* Render = "Phase.Render";
inline constexpr const char* Cleanup = "Phase.Cleanup";
inline constexpr const char* End = "Phase.End";
}

// Where a system wants to run, as "after these / before these" names.
class Placement {
public:
    static Placement InPhase(std::string_view phase); // after the phase anchor, before the next one

    Placement& After(std::string name) { after.push_back(std::move(name)); return *this; }
    Placement& Before(std::string name) { before.push_back(std::move(name)); return *this; }

    std::vector<std::string> after;
    std::vector<std::string> before;
};

// Ordered list of systems. Modules say where they belong and the pipeline trusts them:
// it only sorts the requested constraints into an order. It does not inspect what a system
// reads or writes, so keeping data access consistent is the requesting module's job.
class Pipeline {
public:
    using System = std::function<void(World&, float)>;

    void Add(std::string name, System system, Placement placement);
    void Remove(std::string_view name);

    void Run(World& world, float deltaTime);

    // Resolved order, anchors included. Handy for logs.
    std::vector<std::string> Describe();

private:
    struct Node {
        std::string name;
        System system; // empty for anchors
        Placement placement;
    };

    std::vector<Node> nodes;
    std::vector<std::size_t> order;
    std::vector<Node> pendingAdds; // systems added while running join after the frame
    bool dirty = true;
    bool running = false;

    void Resolve();
};

}
