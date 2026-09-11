#include "systems/ChestManager.h"
#include "ui/VisualStyle.h"

void ChestManager::Reset() {
    for (Chest& chest : chests_) chest.active = false;
    activeCount_ = 0;
}

bool ChestManager::Spawn(Vector2 position) {
    for (Chest& chest : chests_) {
        if (chest.active) continue;
        chest = {position, 22.0f, true};
        ++activeCount_;
        return true;
    }
    return false;
}

bool ChestManager::Update(Vector2 playerPosition, float playerRadius) {
    for (Chest& chest : chests_) {
        if (!chest.active) continue;
        const float dx = chest.position.x - playerPosition.x;
        const float dy = chest.position.y - playerPosition.y;
        const float reach = chest.radius + playerRadius;
        if (dx * dx + dy * dy > reach * reach) continue;
        chest.active = false;
        --activeCount_;
        return true;
    }
    return false;
}

void ChestManager::Draw() const {
    const float time = static_cast<float>(GetTime());
    for (const Chest& chest : chests_) {
        if (!chest.active) continue;
        VisualStyle::DrawChest(chest.position, chest.radius, time);
    }
}
