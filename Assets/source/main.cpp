#include "FirstPersonCamera.h"
#include "GameMessages.h"
#include "GripSync.h"
#include "Hud.h"
#include "LocalPlayer.h"
#include "Scene.h"
#include "Tank.h"
#include "Scrapper/Core/App.h"
#include "Scrapper/Jolt/JoltModule.h"
#include "Scrapper/Net/NetModule.h"
#include "Scrapper/Raylib/Cameras.h"
#include "Scrapper/Raylib/RaylibModule.h"
#include "Scrapper/Raylib/RenderSettings.h"
#include "Scrapper/ScriptEngine/ScriptEngineModule.h"

#include <optional>
#include <string>

using namespace Scrapper;


// No arguments: host a game.  "--join <address>": join one.
int main(int argc, char** argv) {
    const bool joining = argc >= 3 && std::string(argv[1]) == "--join";

    // Fonts/ and Models/ are deployed next to the executable; find them wherever it was launched from.
    ChangeDirectory(GetApplicationDirectory());

    App app;
    WindowConfig window;
    window.title = joining ? "ScrapperEngine - Client" : "ScrapperEngine - Host";
    app.Install<RaylibModule>(window);
    app.Install<JoltModule>();
    NetConfig net;
    net.role = joining ? NetRole::Client : NetRole::Server;
    if (joining) net.address = argv[2];
    app.Install<NetModule>(net);
    app.Install<ScriptEngineModule>();

    World& world = app.GetWorld();
    world.Resource<RenderSettings>().clearColor = { 150, 190, 230, 255 };

    std::optional<Scene> scene = Scene::Build(world);
    if (!scene) return 1;

    GameObject camera = GameObject::Create(world, { 10.0f, 7.0f, 14.0f });
    camera.Add<CameraView3D>(CameraView3D{ 75.0f });
    camera.LookAt({ 0.0f, 1.0f, 0.0f });

    const LocalPlayer& player = GameObject::CreateEmpty(world).AddScript<LocalPlayer>(scene->crew, scene->tank, camera);
    camera.AddScript<FirstPersonCamera>(player);
    Hud::Create(world, player);

    NetSession& session = world.Resource<NetSession>();
    ListenForGrips(world);
    if (session.IsServer()) {
        TankDrive& drive = scene->tank.AddScript<TankDrive>();
        session.On(GameMessages::Shove, [&drive](ConnectionId, ByteReader&) { drive.Shove(); });
        session.On(GameMessages::Drive, [&drive](ConnectionId sender, ByteReader& reader) {
            Vector2 input = { 0.0f, 0.0f };
            if (reader.Read(input)) drive.SetInput(sender, input);
        });
        session.onConnected = [&world, crew = scene->crew](ConnectionId client) {
            AssignCrew(crew, client);
            SendAllGrips(world, client);
        };
        session.onDisconnected = [&world, &drive](ConnectionId client) {
            drive.Forget(client);
            ReleaseGripsOf(world, client);
        };
    }

    app.Run();
    return 0;
}
