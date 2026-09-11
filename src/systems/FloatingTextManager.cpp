#include "systems/FloatingTextManager.h"

#include <algorithm>
#include <cmath>

FloatingTextManager::FloatingTextManager() {
    texts_.reserve(160);
    texts_.resize(160);
}

void FloatingTextManager::Reset() {
    for (FloatingText& text : texts_) text.active = false;
    activeCount_ = 0;
}

void FloatingTextManager::Spawn(Vector2 position, float value, bool critical) {
    for (FloatingText& text : texts_) {
        if (!text.active || text.age > 0.12f) continue;
        const float dx = text.position.x - position.x;
        const float dy = text.position.y - position.y;
        if (dx * dx + dy * dy > 24.0f * 24.0f) continue;
        text.value += value;
        text.critical |= critical;
        text.lifetime = std::max(text.lifetime, 0.72f);
        return;
    }
    for (FloatingText& text : texts_) {
        if (text.active) continue;
        text = {position, value, critical ? 0.95f : 0.72f, 0.0f, critical, true};
        ++activeCount_;
        return;
    }
}

void FloatingTextManager::Update(float deltaTime) {
    for (FloatingText& text : texts_) {
        if (!text.active) continue;
        text.age += deltaTime;
        text.position.y -= deltaTime * (text.critical ? 42.0f : 30.0f);
        if (text.age >= text.lifetime) {
            text.active = false;
            --activeCount_;
        }
    }
}

void FloatingTextManager::Draw() const {
    for (const FloatingText& text : texts_) {
        if (!text.active) continue;
        const float fade = std::clamp(1.0f - text.age / text.lifetime, 0.0f, 1.0f);
        const int fontSize = text.critical ? 22 : 16;
        const char* label = text.critical ? TextFormat("%.0f CRIT!", text.value)
                                          : TextFormat("%.0f", text.value);
        const int width = MeasureText(label, fontSize);
        const Color shadow{8, 8, 14, static_cast<unsigned char>(220.0f * fade)};
        const Color color = text.critical
                                ? Color{255, 226, 75, static_cast<unsigned char>(255.0f * fade)}
                                : Color{235, 245, 255, static_cast<unsigned char>(245.0f * fade)};
        DrawText(label, static_cast<int>(text.position.x - width * 0.5f + 2),
                 static_cast<int>(text.position.y + 2), fontSize, shadow);
        DrawText(label, static_cast<int>(text.position.x - width * 0.5f),
                 static_cast<int>(text.position.y), fontSize, color);
    }
}
