#pragma once

#include "Scrapper/ScriptEngine/GameObject.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Jolt/SpringCharacter.h"


namespace Scrapper::ForJolt {
// Script-side view of the Jolt module's data. Reads come from the last physics step;
// writes become intent (PhysicsCommands, SpringCharacter input) that Jolt applies at its own step.
class Body {
public:
    explicit Body(GameObject gameObject) : gameObject(gameObject) {}

    void AddImpulse(Vector3 impulse);
    void AddAngularImpulse(Vector3 impulse);
    void AddForce(Vector3 force);   // for this frame
    void AddTorque(Vector3 torque); // for this frame
    void SetLinearVelocity(Vector3 velocity);
    Vector3 GetLinearVelocity() const;  // as of the last physics step
    Vector3 GetAngularVelocity() const;

private:
    GameObject gameObject;
    PhysicsCommands& Commands();
};

class Character {
public:
    explicit Character(GameObject gameObject) : gameObject(gameObject) {}

    void SetMoveInput(Vector3 direction); // world space, length <= 1
    void SetFacingYaw(float radians);     // 0 faces -Z
    void Jump();
    void AddStun(float seconds, float factor); // stability drops to factor, recovers over seconds
    void Knockback(Vector3 impulse, float stunSeconds);
    float GetStability() const; // 0 limp .. 1 steady
    bool IsGrounded() const;

private:
    GameObject gameObject;
};

}
