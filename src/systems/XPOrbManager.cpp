#include "systems/XPOrbManager.h"

#include <algorithm>
#include <cmath>

#include "gameplay/Balance.h"

XPOrbManager::XPOrbManager() {
    orbs_.reserve(Balance::XPOrbPoolSize);
    orbs_.resize(Balance::XPOrbPoolSize);
}

void XPOrbManager::Reset() {
    for (XPOrb& orb : orbs_) {
        orb.active = false;
    }
    activeCount_ = 0;
    poolWarningEmitted_ = false;
}

void XPOrbManager::Spawn(Vector2 position, int value) {
    for (XPOrb& orb : orbs_) {
        if (orb.active) {
            continue;
        }
        orb = {position, value, Balance::XPOrbRadius, false, false, true};
        ++activeCount_;
        return;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "XP orb pool full; XP drop skipped");
        poolWarningEmitted_ = true;
    }
}

int XPOrbManager::Update(float deltaTime, Vector2 playerPosition, float playerRadius,
                         float magnetRadius) {
    int collectedXP = 0;
    for (XPOrb& orb : orbs_) {
        if (!orb.active) {
            continue;
        }

        const float dx = playerPosition.x - orb.position.x;
        const float dy = playerPosition.y - orb.position.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared <= magnetRadius * magnetRadius) {
            orb.attracted = true;
        }

        if (orb.attracted && distanceSquared > 0.001f) {
            const float distance = std::sqrt(distanceSquared);
            float speed = 210.0f + (magnetRadius - std::min(distance, magnetRadius)) * 4.0f;
            if (orb.fastAttract) speed *= 3.0f;
            orb.position.x += dx / distance * speed * deltaTime;
            orb.position.y += dy / distance * speed * deltaTime;
        }

        const float collectDistance = playerRadius + orb.radius + 3.0f;
        const float newDx = playerPosition.x - orb.position.x;
        const float newDy = playerPosition.y - orb.position.y;
        if (newDx * newDx + newDy * newDy <= collectDistance * collectDistance) {
            collectedXP += orb.value;
            orb.active = false;
            --activeCount_;
        }
    }
    return collectedXP;
}

void XPOrbManager::AttractAll() {
    for (XPOrb& orb : orbs_) if (orb.active) { orb.attracted = true; orb.fastAttract = true; }
}

void XPOrbManager::Draw() const {
    const float time = static_cast<float>(GetTime());
    for (const XPOrb& orb : orbs_) {
        if (!orb.active) {
            continue;
        }
        const float scale = orb.value >= 20 ? 1.55f : (orb.value >= 8 ? 1.25f : 1.0f);
        const float pulse = 1.0f + std::sin(time * 5.0f + orb.position.x * 0.02f) * 0.12f;
        const float radius = orb.radius * scale * pulse;
        DrawCircleV(orb.position, radius + 6.0f, Color{75, 255, 205, 38});
        DrawPoly(orb.position, orb.value >= 20 ? 8 : (orb.value >= 8 ? 6 : 4),
                 radius, 45.0f + time * 35.0f, Color{80, 245, 190, 255});
        DrawPolyLinesEx(orb.position, 4, radius * 0.5f, -time * 45.0f, 1.5f, WHITE);
    }
}
