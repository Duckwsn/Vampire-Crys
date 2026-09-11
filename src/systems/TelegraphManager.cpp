#include "systems/TelegraphManager.h"

#include <algorithm>
#include <cmath>

#include "entities/Player.h"

namespace {
float LengthSquared(Vector2 value) { return value.x * value.x + value.y * value.y; }
Vector2 Normalize(Vector2 value) {
    const float length = std::sqrt(std::max(0.0001f, LengthSquared(value)));
    return {value.x / length, value.y / length};
}
float DistanceToSegmentSquared(Vector2 point, Vector2 start, Vector2 end) {
    const Vector2 segment{end.x - start.x, end.y - start.y};
    const float denominator = std::max(0.0001f, LengthSquared(segment));
    const float projection = std::clamp(((point.x - start.x) * segment.x +
                                         (point.y - start.y) * segment.y) / denominator,
                                        0.0f, 1.0f);
    const Vector2 closest{start.x + segment.x * projection, start.y + segment.y * projection};
    return LengthSquared({point.x - closest.x, point.y - closest.y});
}
}

TelegraphManager::TelegraphManager() {
    telegraphs_.reserve(48);
    telegraphs_.resize(48);
}

void TelegraphManager::Reset() {
    for (Telegraph& telegraph : telegraphs_) telegraph.active = false;
    activeCount_ = 0;
    poolWarningEmitted_ = false;
}

bool TelegraphManager::Spawn(const Telegraph& specification) {
    for (Telegraph& telegraph : telegraphs_) {
        if (telegraph.active) continue;
        telegraph = specification;
        telegraph.active = true;
        telegraph.elapsed = 0.0f;
        telegraph.tickTimer = 0.0f;
        ++activeCount_;
        return true;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "Telegraph pool full; effect skipped");
        poolWarningEmitted_ = true;
    }
    return false;
}

float TelegraphManager::Update(float deltaTime, Player& player) {
    float damageTaken = 0.0f;
    for (Telegraph& telegraph : telegraphs_) {
        if (!telegraph.active) continue;
        telegraph.elapsed += deltaTime;
        telegraph.tickTimer -= deltaTime;
        if (telegraph.dangerous && telegraph.tickTimer <= 0.0f &&
            Hits(telegraph, player.Position(), player.Radius())) {
            if (player.TakeDamage(telegraph.damage)) damageTaken += player.LastDamageTaken();
            telegraph.tickTimer += telegraph.tickInterval;
        }
        if (telegraph.elapsed >= telegraph.duration) {
            telegraph.active = false;
            --activeCount_;
        }
    }
    return damageTaken;
}

bool TelegraphManager::CircleHits(const Telegraph& shape, Vector2 point, float pointRadius) {
    const float dx = point.x - shape.position.x;
    const float dy = point.y - shape.position.y;
    const float reach = shape.radius + pointRadius;
    return dx * dx + dy * dy <= reach * reach;
}

bool TelegraphManager::ConeHits(const Telegraph& shape, Vector2 point, float pointRadius) {
    const Vector2 delta{point.x - shape.position.x, point.y - shape.position.y};
    const float distance = std::sqrt(std::max(0.0001f, LengthSquared(delta)));
    if (distance > shape.range + pointRadius) return false;
    const Vector2 direction = Normalize(shape.direction);
    const float dot = std::clamp((delta.x * direction.x + delta.y * direction.y) / distance,
                                 -1.0f, 1.0f);
    const float tolerance = std::asin(std::min(1.0f, pointRadius / distance));
    return std::acos(dot) <= shape.angleDegrees * 0.5f * DEG2RAD + tolerance;
}

bool TelegraphManager::LineHits(const Telegraph& shape, Vector2 point, float pointRadius) {
    const Vector2 direction = Normalize(shape.direction);
    const Vector2 end{shape.position.x + direction.x * shape.range,
                      shape.position.y + direction.y * shape.range};
    const float reach = shape.width * 0.5f + pointRadius;
    return DistanceToSegmentSquared(point, shape.position, end) <= reach * reach;
}

bool TelegraphManager::Hits(const Telegraph& shape, Vector2 point, float pointRadius) const {
    if (shape.type == TelegraphType::Circle) return CircleHits(shape, point, pointRadius);
    if (shape.type == TelegraphType::Cone) return ConeHits(shape, point, pointRadius);
    return LineHits(shape, point, pointRadius);
}

void TelegraphManager::Draw() const {
    for (const Telegraph& telegraph : telegraphs_) {
        if (!telegraph.active) continue;
        const float progress = telegraph.duration > 0.0f
                                   ? std::clamp(telegraph.elapsed / telegraph.duration, 0.0f, 1.0f)
                                   : 1.0f;
        const unsigned char alpha = telegraph.dangerous ? 115 : static_cast<unsigned char>(28 + progress * 75);
        const Color fill{telegraph.color.r, telegraph.color.g, telegraph.color.b, alpha};
        const Color edge{telegraph.color.r, telegraph.color.g, telegraph.color.b,
                         static_cast<unsigned char>(telegraph.dangerous ? 245 : 150 + progress * 100)};
        if (telegraph.type == TelegraphType::Circle) {
            DrawCircleV(telegraph.position, telegraph.radius, fill);
            DrawCircleLines(static_cast<int>(telegraph.position.x), static_cast<int>(telegraph.position.y),
                            telegraph.radius, edge);
            const float scanRadius = telegraph.radius * (0.25f + 0.75f * progress);
            DrawRing(telegraph.position, std::max(0.0f, scanRadius - 2.0f), scanRadius + 2.0f,
                     0.0f, 360.0f, 40, edge);
        } else if (telegraph.type == TelegraphType::Cone) {
            const float heading = std::atan2(telegraph.direction.y, telegraph.direction.x) * RAD2DEG;
            DrawCircleSector(telegraph.position, telegraph.range,
                             heading - telegraph.angleDegrees * 0.5f,
                             heading + telegraph.angleDegrees * 0.5f, 24, fill);
            DrawCircleSectorLines(telegraph.position, telegraph.range,
                                  heading - telegraph.angleDegrees * 0.5f,
                                  heading + telegraph.angleDegrees * 0.5f, 24, edge);
            const Vector2 pulseEnd{telegraph.position.x + telegraph.direction.x * telegraph.range * progress,
                                   telegraph.position.y + telegraph.direction.y * telegraph.range * progress};
            DrawLineEx(telegraph.position, pulseEnd, 3.0f, edge);
        } else {
            const Vector2 direction = Normalize(telegraph.direction);
            const Vector2 end{telegraph.position.x + direction.x * telegraph.range,
                              telegraph.position.y + direction.y * telegraph.range};
            DrawLineEx(telegraph.position, end, telegraph.width, fill);
            DrawLineEx(telegraph.position, end, telegraph.dangerous ? 3.0f : 2.0f, edge);
            const Vector2 scan{telegraph.position.x + direction.x * telegraph.range * progress,
                               telegraph.position.y + direction.y * telegraph.range * progress};
            DrawCircleV(scan, telegraph.dangerous ? 7.0f : 4.0f, edge);
        }
    }
}
