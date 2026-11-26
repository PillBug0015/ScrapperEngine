#pragma once

#include "Scrapper/Core/Module.h"


namespace Scrapper {
namespace ScriptSystem {
inline constexpr const char* Update = "Scripts.Update";
inline constexpr const char* LateUpdate = "Scripts.LateUpdate";
inline constexpr const char* Destroy = "Scripts.Destroy";
}

class ScriptEngineModule : public Module {
public:
    void Install(World& world) override;
    void Uninstall(World& world) override;
};

}
