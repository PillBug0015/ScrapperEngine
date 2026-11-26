#pragma once

#include "Scrapper/Assets/Model.h"
#include "Scrapper/Net/NetTypes.h"
#include "Scrapper/ScriptEngine/Script.h"

#include <memory>
#include <unordered_map>


// The model's COL_ pieces become one dynamic body; its markers become named children that ride along.
Scrapper::GameObject SpawnTank(Scrapper::World& world, const std::shared_ptr<const Scrapper::ModelAsset>& model, Vector3 position);

// Server only. Everyone's drive input adds up on the same tank, so the crew has to agree.
class TankDrive : public Scrapper::Script {
public:
    void SetInput(Scrapper::ConnectionId from, Vector2 input); // x: turn right, y: forward
    void Forget(Scrapper::ConnectionId from);
    void Shove();
    void Update(float deltaTime) override;

private:
    std::unordered_map<Scrapper::ConnectionId, Vector2> inputs;
};
