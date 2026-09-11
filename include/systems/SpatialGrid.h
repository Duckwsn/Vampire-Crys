#pragma once

#include <vector>

#include "raylib.h"

struct Enemy;

class SpatialGrid {
public:
    static constexpr float CellSize = 128.0f;
    static constexpr int Columns = 32;
    static constexpr int Rows = 32;

    SpatialGrid();
    void Clear();
    void Rebuild(const std::vector<Enemy>& enemies);
    void QueryCircle(Vector2 position, float radius, std::vector<int>& results) const;

    int ActiveCellCount() const { return activeCellCount_; }
    int EntryCount() const { return entryCount_; }

private:
    int CellX(float worldX) const;
    int CellY(float worldY) const;
    int Index(int x, int y) const { return y * Columns + x; }

    std::vector<std::vector<int>> cells_;
    int activeCellCount_ = 0;
    int entryCount_ = 0;
};
