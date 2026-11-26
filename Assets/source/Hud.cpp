#include "Hud.h"
#include "Scrapper/Jolt/SpringCharacter.h"
#include "Scrapper/Net/NetSession.h"
#include "Scrapper/Raylib/RectLayout.h"
#include "Scrapper/Raylib/Renderers.h"

#include <string>

using namespace Scrapper;


void Hud::Create(World& world, const LocalPlayer& player) {
    GameObject status = GameObject::Create(world);
    status.Add<RectLayout>(RectLayout{ { 24.0f, 20.0f } });
    status.Add<TextRenderer>(TextRenderer{ "", 22, WHITE, 100 });

    GameObject crosshair = GameObject::Create(world);
    crosshair.Add<RectLayout>(RectLayout{ { 0.0f, 0.0f }, { 0.5f, 0.5f }, { 0.5f, 0.5f }, { 6.0f, 6.0f } });
    crosshair.Add<RectRenderer>(RectRenderer{ { 6.0f, 6.0f }, Color{ 255, 255, 255, 200 } });

    GameObject::CreateEmpty(world).AddScript<Hud>(player, status);
}

void Hud::Update(float) {
    const NetSession& session = GetWorld().Resource<NetSession>();
    std::string role;
    if (session.IsServer()) role = TextFormat("HOST  clients %d", static_cast<int>(session.ClientCount()));
    else if (session.IsConnected()) role = TextFormat("CLIENT #%u", session.LocalId());
    else role = TextFormat("CLIENT  connecting to %s ...", session.Address().c_str());

    const GameObject body = player.Body();
    const float stability = body ? body.Get<SpringCharacter>().stability : 0.0f;
    status.Get<TextRenderer>().text = TextFormat(
        "%s   stability %.2f   damping x%.2f\n"
        "click: mouse look  TAB: free mouse  WASD SPACE  LMB/RMB: reach  Q: fall over  ARROWS: drive  E: shove tank",
        role.c_str(), stability, GetWorld().Resource<CharacterSpringScale>().damping);
}
