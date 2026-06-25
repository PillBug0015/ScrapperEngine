#include "Engine.h"
#include "GameObject.h"
#include "GameManager.h"
#include "IKChain.h"
#include <iostream>
#include <string>

using namespace ScrapperEngine;

int main() {
    std::cout << "==================================================\n";
    std::cout << "        2D TOY ENGINE SANDBOX LAUNCHER\n";
    std::cout << "==================================================\n";
    std::cout << "Select a game or demo to run:\n";
    std::cout << " [1] JRPG & Text RPG Game (TextRPGKit)\n";
    std::cout << " [2] 2D Inverse Kinematics Demo (IKToy)\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "Enter your choice (1 or 2, default is 1): ";

    std::string input;
    std::getline(std::cin, input);

    int choice = 1;
    if (input == "2") {
        choice = 2;
    }

    if (choice == 2) {
        std::cout << "\n>> Launching 2D Inverse Kinematics (IKToy) Sandbox...\n";

        Engine engine(1024, 768, "2D Inverse Kinematics (CCD vs FABRIK) Sandbox");
        if (engine.Initialize()) {
            // Create a root GameObject for the IK Chain.
            GameObject* ikBaseObj = engine.Instantiate2D({ 512.0f, 450.0f });

            // Attach the IKChain component
            ikBaseObj->AddComponent<IKChain>();

            // Run!
            engine.Run();
        }
    }
    else {
        std::cout << "\n>> Launching JRPG & Text RPG (TextRPGKit) Sandbox...\n";

        Engine engine(1024, 768, "JRPG Text RPG Sandbox");
        if (engine.Initialize()) {
            // Create a UI-root GameObject for the GameManager.
            GameObject* gameManagerObj = engine.InstantiateRect({ 0.0f, 0.0f }, { 1024.0f, 768.0f });
            
            // Attach the GameManager component
            gameManagerObj->AddComponent<GameManager>();

            // Run!
            engine.Run();
        }
    }

    std::cout << "\n>> Sandbox terminated. Goodbye.\n";
    return 0;
}
