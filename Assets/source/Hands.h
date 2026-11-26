#pragma once

#include "Scrapper/Jolt/Ragdoll.h"
#include "Scrapper/ScriptEngine/GameObject.h"


// One hand of a body this peer owns: while `held`, the arm reaches for `reachPoint`, and once its tip
// touches something it grips there, until let go or knocked out of it.
void UpdateHand(Scrapper::World& world, Scrapper::GameObject body, Scrapper::PartId arm, bool held, Vector3 reachPoint);
