#include "LocalPlayer.h"
#include "GameMessages.h"
#include "Tank.h"
#include "Scrapper/Net/NetSession.h"
#include "Scrapper/ScriptEngine/ForJolt.h"

#include <cmath>

using namespace Scrapper;

namespace {
constexpr float MouseSensitivity = 0.0025f; // radians per pixel
constexpr float MaxPitch = 1.45f;

Vector3 Flat(float yaw, float forward, float right) {
    const Vector3 f = { -std::sin(yaw), 0.0f, -std::cos(yaw) };
    const Vector3 r = { std::cos(yaw), 0.0f, -std::sin(yaw) };
    const Vector3 move = Vector3Add(Vector3Scale(f, forward), Vector3Scale(r, right));
    return Vector3LengthSqr(move) > 1.0f ? Vector3Normalize(move) : move;
}

float Axis(int positive, int negative) {
    return static_cast<float>(IsKeyDown(positive)) - static_cast<float>(IsKeyDown(negative));
}
}


void LocalPlayer::Update(float) {
    TakeBody(FindLocalCrew(crew, GetWorld().Resource<NetSession>()));
    Look();
    Move();
    Drive();
}

// Ownership can change any frame (joining, the server taking a body back): follow it.
void LocalPlayer::TakeBody(GameObject next) {
    if (next == body) return;
    if (body) SetHeadVisible(body, true);
    body = next;
    if (body) SetHeadVisible(body, false); // the camera sits inside it
}

// Click to capture the mouse, Tab to let it go.
void LocalPlayer::Look() {
    if (IsCursorHidden()) {
        const Vector2 delta = GetMouseDelta();
        yaw -= delta.x * MouseSensitivity;
        pitch = Clamp(pitch - delta.y * MouseSensitivity, -MaxPitch, MaxPitch);
    }
    if (IsKeyPressed(KEY_TAB)) EnableCursor();
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) DisableCursor();
}

void LocalPlayer::Move() {
    if (!body) return;
    ForJolt::Character character(body);
    character.SetFacingYaw(yaw);
    character.SetMoveInput(Flat(yaw, Axis(KEY_W, KEY_S), Axis(KEY_D, KEY_A)));
    if (IsKeyPressed(KEY_SPACE)) character.Jump();
    if (IsKeyPressed(KEY_Q)) {
        const Vector3 back = Flat(yaw, -1.0f, 0.0f);
        character.Knockback({ back.x * 320.0f, 360.0f, back.z * 320.0f }, 2.0f);
    }

    // Debug knob over every body this peer simulates.
    CharacterSpringScale& springs = GetWorld().Resource<CharacterSpringScale>();
    if (IsKeyPressed(KEY_LEFT_BRACKET)) springs.damping *= 0.8f;
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) springs.damping *= 1.25f;
}

void LocalPlayer::Drive() {
    NetSession& session = GetWorld().Resource<NetSession>();
    const Vector2 input = { Axis(KEY_RIGHT, KEY_LEFT), Axis(KEY_UP, KEY_DOWN) };
    const bool shove = IsKeyPressed(KEY_E);

    if (session.IsServer()) {
        TankDrive* drive = tank.GetScript<TankDrive>();
        drive->SetInput(ServerOwner, input);
        if (shove) drive->Shove();
        return;
    }
    ByteWriter payload;
    payload.Write(input);
    session.SendToServer(NetChannel::Unreliable, GameMessages::Drive, payload);
    if (shove) session.SendToServer(NetChannel::Reliable, GameMessages::Shove);
}
