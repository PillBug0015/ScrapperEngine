#pragma once

#include "Crew.h"
#include "Scrapper/ScriptEngine/Script.h"


// This peer's hands on the game: mouse look and keys drive the crew member it owns directly
// (it simulates that body itself); tank controls go to the server.
class LocalPlayer : public Scrapper::Script {
public:
    LocalPlayer(CrewList crew, Scrapper::GameObject tank) : crew(crew), tank(tank) {}

    Scrapper::GameObject Body() const { return body; } // invalid until this peer owns a crew member
    float Yaw() const { return yaw; }     // radians around +Y; 0 looks down -Z
    float Pitch() const { return pitch; } // radians; positive looks up

    void Update(float deltaTime) override;

private:
    CrewList crew;
    Scrapper::GameObject tank;
    Scrapper::GameObject body;
    float yaw = 0.0f;
    float pitch = 0.0f;

    void TakeBody(Scrapper::GameObject next);
    void Look();
    void Move();
    void Drive();
};
