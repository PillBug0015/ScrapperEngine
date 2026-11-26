#include "Scrapper/ScriptEngine/ForJolt.h"


namespace Scrapper::ForJolt {
PhysicsCommands& Body::Commands() {
    return gameObject.GetWorld().Registry().get_or_emplace<PhysicsCommands>(gameObject.Entity());
}

void Body::AddImpulse(Vector3 impulse) { Commands().impulse = Vector3Add(Commands().impulse, impulse); }
void Body::AddAngularImpulse(Vector3 impulse) { Commands().angularImpulse = Vector3Add(Commands().angularImpulse, impulse); }
void Body::AddForce(Vector3 force) { Commands().force = Vector3Add(Commands().force, force); }
void Body::AddTorque(Vector3 torque) { Commands().torque = Vector3Add(Commands().torque, torque); }

void Body::SetLinearVelocity(Vector3 velocity) {
    Commands().overrideLinearVelocity = true;
    Commands().linearVelocity = velocity;
}

Vector3 Body::GetLinearVelocity() const {
    const PhysicsBody* body = gameObject.TryGet<PhysicsBody>();
    return body != nullptr ? body->linearVelocity : Vector3{ 0.0f, 0.0f, 0.0f };
}

Vector3 Body::GetAngularVelocity() const {
    const PhysicsBody* body = gameObject.TryGet<PhysicsBody>();
    return body != nullptr ? body->angularVelocity : Vector3{ 0.0f, 0.0f, 0.0f };
}

void Character::SetMoveInput(Vector3 direction) { gameObject.Get<SpringCharacter>().moveInput = direction; }
void Character::SetFacingYaw(float radians) { gameObject.Get<SpringCharacter>().facingYaw = radians; }
void Character::Jump() { gameObject.Get<SpringCharacter>().jumpRequested = true; }
void Character::AddStun(float seconds, float factor) { Characters::AddStun(gameObject.Get<SpringCharacter>(), seconds, factor); }

void Character::Knockback(Vector3 impulse, float stunSeconds) {
    PhysicsCommands& commands = gameObject.GetWorld().Registry().get_or_emplace<PhysicsCommands>(gameObject.Entity());
    Characters::Knockback(gameObject.Get<SpringCharacter>(), commands, impulse, stunSeconds);
}

float Character::GetStability() const { return gameObject.Get<SpringCharacter>().stability; }
bool Character::IsGrounded() const { return gameObject.Get<SpringCharacter>().grounded; }

}
