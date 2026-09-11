#pragma once

#include <vector>

#include "raylib.h"

struct FloatingText {
    Vector2 position{};
    float value = 0.0f;
    float lifetime = 0.0f;
    float age = 0.0f;
    bool critical = false;
    bool active = false;
};

class FloatingTextManager {
public:
    FloatingTextManager();
    void Reset();
    void Spawn(Vector2 position, float value, bool critical);
    void Update(float deltaTime);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }
private:
    std::vector<FloatingText> texts_;
    int activeCount_ = 0;
};
