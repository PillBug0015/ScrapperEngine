#include "Scrapper/Core/Pipeline.h"

#include <algorithm>
#include <cstdio>
#include <queue>
#include <unordered_map>


namespace Scrapper {
namespace {
constexpr const char* PhaseOrder[] = {
    Phase::Input, Phase::Update, Phase::PreSimulate, Phase::Simulate, Phase::PostSimulate,
    Phase::LateUpdate, Phase::Render, Phase::Cleanup, Phase::End,
};
}

Placement Placement::InPhase(std::string_view phase) {
    Placement placement;
    placement.after.emplace_back(phase);

    const auto it = std::find_if(std::begin(PhaseOrder), std::end(PhaseOrder),
                                 [&](const char* name) { return phase == name; });
    if (it != std::end(PhaseOrder) && std::next(it) != std::end(PhaseOrder)) {
        placement.before.emplace_back(*std::next(it));
    }
    return placement;
}

void Pipeline::Add(std::string name, System system, Placement placement) {
    Node node{ std::move(name), std::move(system), std::move(placement) };
    if (running) {
        pendingAdds.push_back(std::move(node));
        return;
    }
    nodes.push_back(std::move(node));
    dirty = true;
}

void Pipeline::Remove(std::string_view name) {
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&](const Node& node) { return node.name == name; }),
                nodes.end());
    dirty = true;
}

void Pipeline::Resolve() {
    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        index.emplace(nodes[i].name, i);
    }

    std::vector<std::vector<std::size_t>> edges(nodes.size());
    std::vector<int> incoming(nodes.size(), 0);
    const auto link = [&](std::size_t from, std::size_t to) {
        edges[from].push_back(to);
        ++incoming[to];
    };

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (const std::string& name : nodes[i].placement.after) {
            if (auto it = index.find(name); it != index.end()) link(it->second, i);
            else std::fprintf(stderr, "[Pipeline] '%s' wants to run after unknown '%s'\n", nodes[i].name.c_str(), name.c_str());
        }
        for (const std::string& name : nodes[i].placement.before) {
            if (auto it = index.find(name); it != index.end()) link(i, it->second);
            else std::fprintf(stderr, "[Pipeline] '%s' wants to run before unknown '%s'\n", nodes[i].name.c_str(), name.c_str());
        }
    }

    // Kahn's algorithm; ties go to whoever was added first, so the result is stable.
    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (incoming[i] == 0) ready.push(i);
    }

    order.clear();
    while (!ready.empty()) {
        const std::size_t current = ready.top();
        ready.pop();
        order.push_back(current);
        for (std::size_t next : edges[current]) {
            if (--incoming[next] == 0) ready.push(next);
        }
    }

    if (order.size() != nodes.size()) {
        std::fprintf(stderr, "[Pipeline] placement cycle; the remaining systems run in insertion order\n");
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (incoming[i] > 0) order.push_back(i);
        }
    }

    dirty = false;
}

void Pipeline::Run(World& world, float deltaTime) {
    if (dirty) {
        Resolve();
    }

    running = true;
    for (std::size_t i : order) {
        if (nodes[i].system) {
            nodes[i].system(world, deltaTime);
        }
    }
    running = false;

    if (!pendingAdds.empty()) {
        for (Node& node : pendingAdds) nodes.push_back(std::move(node));
        pendingAdds.clear();
        dirty = true;
    }
}

std::vector<std::string> Pipeline::Describe() {
    if (dirty) {
        Resolve();
    }
    std::vector<std::string> names;
    for (std::size_t i : order) names.push_back(nodes[i].name);
    return names;
}

}
