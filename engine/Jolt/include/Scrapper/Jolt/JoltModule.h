#pragma once

#include "Scrapper/Core/Module.h"


namespace Scrapper {
namespace JoltSystem {
inline constexpr const char* Simulate = "Jolt.Simulate";
}

// Turns RigidBody / SpringCharacter data into Jolt bodies and steps them at a fixed rate.
class JoltModule : public Module {
public:
    void Install(World& world) override;
    void Uninstall(World& world) override;
};

}
