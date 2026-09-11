#include "systems/PickupManager.h"
#include "ui/VisualStyle.h"

PickupManager::PickupManager() {
    pickups_.reserve(100);
    pickups_.resize(100);
}

void PickupManager::Reset() {
    for (Pickup& pickup : pickups_) pickup.active = false;
    activeCount_ = 0;
    poolWarningEmitted_ = false;
}

bool PickupManager::Spawn(Vector2 position, PickupType type) {
    for (Pickup& pickup : pickups_) {
        if (pickup.active) continue;
        pickup = {position, type, 12.0f, 30.0f, true};
        ++activeCount_;
        return true;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "Pickup pool full; further drops are skipped");
        poolWarningEmitted_ = true;
    }
    return false;
}

PickupResult PickupManager::Update(float deltaTime, Vector2 playerPosition, float playerRadius) {
    PickupResult result{};
    for (Pickup& pickup : pickups_) {
        if (!pickup.active) continue;
        pickup.lifetime -= deltaTime;
        const float dx = pickup.position.x - playerPosition.x;
        const float dy = pickup.position.y - playerPosition.y;
        const float reach = pickup.radius + playerRadius;
        if (pickup.lifetime > 0.0f && dx * dx + dy * dy > reach * reach) continue;
        if (pickup.lifetime > 0.0f) {
            if (pickup.type == PickupType::Health) ++result.health;
            else if (pickup.type == PickupType::Magnet) ++result.magnet;
            else ++result.bomb;
        }
        pickup.active = false;
        --activeCount_;
    }
    return result;
}

void PickupManager::Draw() const {
    const float time = static_cast<float>(GetTime());
    for (const Pickup& pickup : pickups_) {
        if (!pickup.active) continue;
        VisualStyle::DrawPickupIcon(static_cast<int>(pickup.type), pickup.position,
                                    pickup.radius, time);
    }
}
