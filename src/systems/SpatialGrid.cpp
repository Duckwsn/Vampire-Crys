#include "systems/SpatialGrid.h"

#include <algorithm>
#include <cmath>

#include "gameplay/Balance.h"
#include "systems/EnemyManager.h"

SpatialGrid::SpatialGrid() {
    cells_.resize(static_cast<std::size_t>(Columns * Rows));
    for (auto& cell : cells_) cell.reserve(12);
}

void SpatialGrid::Clear() {
    for (auto& cell : cells_) cell.clear();
    activeCellCount_ = 0;
    entryCount_ = 0;
}

void SpatialGrid::Rebuild(const std::vector<Enemy>& enemies) {
    Clear();
    for (std::size_t index = 0; index < enemies.size(); ++index) {
        if (!enemies[index].active) continue;
        auto& cell = cells_[static_cast<std::size_t>(Index(CellX(enemies[index].position.x),
                                                           CellY(enemies[index].position.y)))];
        if (cell.empty()) ++activeCellCount_;
        cell.push_back(static_cast<int>(index));
        ++entryCount_;
    }
}

void SpatialGrid::QueryCircle(Vector2 position, float radius, std::vector<int>& results) const {
    results.clear();
    const int minX = CellX(position.x - radius);
    const int maxX = CellX(position.x + radius);
    const int minY = CellY(position.y - radius);
    const int maxY = CellY(position.y + radius);
    for (int y = minY; y <= maxY; ++y)
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[static_cast<std::size_t>(Index(x, y))];
            results.insert(results.end(), cell.begin(), cell.end());
        }
}

int SpatialGrid::CellX(float worldX) const {
    return std::clamp(static_cast<int>(std::floor((worldX - Balance::Arena.x) / CellSize)),
                      0, Columns - 1);
}

int SpatialGrid::CellY(float worldY) const {
    return std::clamp(static_cast<int>(std::floor((worldY - Balance::Arena.y) / CellSize)),
                      0, Rows - 1);
}
