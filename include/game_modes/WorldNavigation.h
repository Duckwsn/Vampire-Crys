#pragma once

#include "raylib.h"

class WorldNavigation {
public:
    virtual ~WorldNavigation() = default;
    virtual bool IsWalkable(Vector2 position, float radius = 0.0f) const = 0;
    virtual Vector2 ConstrainMovement(Vector2 from, Vector2 desired, float radius) const = 0;
    virtual Vector2 FindNearestWalkablePosition(Vector2 position, float radius = 0.0f) const = 0;
    virtual Vector2 DirectionToward(Vector2 from, Vector2 target) const = 0;
    virtual Vector2 ConstrainCamera(Vector2 target, Vector2 viewportHalfSize) const = 0;
    virtual Rectangle Bounds() const = 0;
    virtual void DrawBackground() const = 0;
    virtual void DrawForeground(Vector2) const {}
    virtual void DrawDebug() const = 0;
};
