#pragma once

#include "Entity.h"
#include "Tile.h"
#include "World.h"

#include <ftxui/dom/elements.hpp>

class Renderer {
public:
    ftxui::Element RenderMenu() const;
    ftxui::Element RenderWorld(const World& world) const;

private:
    ftxui::Element RenderCell(const Tile& tile, const Entity* entity) const;
    ftxui::Element RenderEntity(const Entity& entity) const;
    ftxui::Element RenderTile(const Tile& tile) const;
};
