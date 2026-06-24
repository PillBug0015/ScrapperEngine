#pragma once

#include "Component.h"

namespace MyMudGame {

class GameManager : public ScrapperEngine::Component {
public:
    GameManager() = default;
    virtual ~GameManager() = default;

    void Start() override;
    
    void Update(float deltaTime) override;
};

} 
