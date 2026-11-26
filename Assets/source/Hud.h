#pragma once

#include "LocalPlayer.h"


// Status line and crosshair.
class Hud : public Scrapper::Script {
public:
    static void Create(Scrapper::World& world, const LocalPlayer& player);

    Hud(const LocalPlayer& player, Scrapper::GameObject status) : player(player), status(status) {}
    void Update(float deltaTime) override;

private:
    const LocalPlayer& player;
    Scrapper::GameObject status;
};
