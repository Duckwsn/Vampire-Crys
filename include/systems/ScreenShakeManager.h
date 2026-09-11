#pragma once

#include "raylib.h"

class ScreenShakeManager {
public:
    void Reset();
    void Add(float magnitude, float duration = 0.20f);
    void Update(float deltaTime);
    Vector2 Offset(float intensity = 1.0f) const;
    float Magnitude() const { return magnitude_; }
private:
    float magnitude_ = 0.0f;
    float remaining_ = 0.0f;
    float duration_ = 0.0f;
    float phase_ = 0.0f;
};
