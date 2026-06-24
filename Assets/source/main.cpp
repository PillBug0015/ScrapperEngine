#include "Camera2DComponent.h"
#include "Engine.h"
#include "GameManager.h"

int main()
{
    Engine engine(960, 540, "MyMudGame");

    if (!engine.Initialize())
    {
        return 1;
    }

    GameObject* gameRoot = engine.Instantiate2D();
    gameRoot->AddComponent<Camera2DComponent>();
    gameRoot->AddComponent<mud::GameManager>();

    engine.Run();

    return 0;
}
