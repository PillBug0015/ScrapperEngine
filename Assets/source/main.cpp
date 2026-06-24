#include "Scrapper.h"

int main()
{
    ScrapperEngine::Engine engine(960, 540, "MyMudGame - Scrapper Engine", 960, 540);

    if (engine.Initialize())
    {
        engine.Run();
        Vector2 position = {960 * 0.5, 540 * 0.5};
        ScrapperEngine::GameObject* title = engine.InstantiateRect(position);
        title->AddComponent<ScrapperEngine::TextRenderer>("Bruh");
    }
}
