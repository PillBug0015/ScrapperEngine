#include "Scrapper.h"
#include "GameDatabase/GameDatabase.h"
#include "GameManager/GameManager.h"
#include <iostream>

int main()
{
    ScrapperEngine::Engine engine(960, 540, "MyMudGame - Scrapper Engine", 960, 540);

    if (engine.Initialize())
    {

        engine.Run();
    }
}
