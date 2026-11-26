#pragma once


namespace Scrapper {
class World;

// A module adds its components' behaviour to a World: resources, systems, registry hooks.
// Uninstall must release everything Install acquired (backend objects, signal connections),
// because the registry outlives the module.
class Module {
public:
    virtual ~Module() = default;
    virtual void Install(World& world) = 0;
    virtual void Uninstall(World& world) { (void)world; }
};

}
