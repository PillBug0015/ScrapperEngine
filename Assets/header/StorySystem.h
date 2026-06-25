#pragma once

#include <string>

class GameManager;

class StorySystem {
public:
    StorySystem() = default;
    ~StorySystem() = default;

    void TriggerStory(const std::string& storyId, GameManager* game);
};
