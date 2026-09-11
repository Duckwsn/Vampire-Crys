#pragma once

#include <vector>

#include "raylib.h"

class Player;

enum class TelegraphType { Circle, Cone, Line };

struct Telegraph {
    TelegraphType type = TelegraphType::Circle;
    Vector2 position{};
    Vector2 direction{1.0f, 0.0f};
    float radius = 0.0f;
    float range = 0.0f;
    float angleDegrees = 0.0f;
    float width = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
    float damage = 0.0f;
    float tickInterval = 0.25f;
    float tickTimer = 0.0f;
    Color color = RED;
    bool dangerous = false;
    bool active = false;
};

class TelegraphManager {
public:
    TelegraphManager();
    void Reset();
    bool Spawn(const Telegraph& specification);
    float Update(float deltaTime, Player& player);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }

    static bool CircleHits(const Telegraph& shape, Vector2 point, float pointRadius);
    static bool ConeHits(const Telegraph& shape, Vector2 point, float pointRadius);
    static bool LineHits(const Telegraph& shape, Vector2 point, float pointRadius);

private:
    bool Hits(const Telegraph& shape, Vector2 point, float pointRadius) const;
    std::vector<Telegraph> telegraphs_;
    int activeCount_ = 0;
    bool poolWarningEmitted_ = false;
};
