#pragma once

#include "LocalPlayer.h"


// Rides in the local player's head. While steady it holds the horizon and pure mouse look;
// as stability drops it follows the tumbling body instead, so a stun is felt through the eyes.
class FirstPersonCamera : public Scrapper::Script {
public:
    explicit FirstPersonCamera(const LocalPlayer& player) : player(player) {}

    void LateUpdate(float deltaTime) override;

private:
    const LocalPlayer& player;
};
