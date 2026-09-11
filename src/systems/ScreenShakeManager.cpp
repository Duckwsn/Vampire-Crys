#include "systems/ScreenShakeManager.h"

#include <algorithm>
#include <cmath>

void ScreenShakeManager::Reset() {
    magnitude_ = 0.0f;
    remaining_ = 0.0f;
    duration_ = 0.0f;
    phase_ = 0.0f;
}

void ScreenShakeManager::Add(float magnitude, float duration) {
    if (magnitude <= magnitude_ && remaining_ > duration * 0.4f) return;
    magnitude_ = std::max(magnitude_, magnitude);
    remaining_ = std::max(remaining_, duration);
    duration_ = std::max(duration_, duration);
}

void ScreenShakeManager::Update(float deltaTime) {
    phase_ += deltaTime * 73.0f;
    remaining_ = std::max(0.0f, remaining_ - deltaTime);
    if (remaining_ <= 0.0f) { magnitude_ = 0.0f; duration_ = 0.0f; }
}

Vector2 ScreenShakeManager::Offset(float intensity) const {
    if (remaining_ <= 0.0f || duration_ <= 0.0f || intensity <= 0.0f) return {};
    const float falloff = remaining_ / duration_;
    const float amount = magnitude_ * falloff * std::clamp(intensity, 0.0f, 1.0f);
    return {std::sin(phase_ * 1.7f) * amount, std::cos(phase_ * 2.3f) * amount};
}
