#include "game_modes/ExpeditionMap.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {
float LengthSquared(Vector2 value) { return value.x * value.x + value.y * value.y; }
Vector2 Normalize(Vector2 value) {
    const float length = std::sqrt(LengthSquared(value));
    return length > 0.001f ? Vector2{value.x / length, value.y / length} : Vector2{};
}
unsigned int CellHash(int x, int y) {
    unsigned int value = static_cast<unsigned int>(x * 374761393 + y * 668265263);
    value = (value ^ (value >> 13u)) * 1274126177u;
    return value ^ (value >> 16u);
}
}

bool ExpeditionMap::Load(const ExpeditionMapDefinition& definition) {
    id_ = definition.id;
    name_ = definition.name;
    height_ = static_cast<int>(definition.layout.size());
    width_ = 0;
    for (const std::string& row : definition.layout)
        width_ = std::max(width_, static_cast<int>(row.size()));
    if (width_ <= 0 || height_ <= 0) return false;
    cells_.assign(static_cast<std::size_t>(width_ * height_), '.');
    spawnPoints_.clear();
    walkableCount_ = 0;
    const Vector2 origin{-width_ * CellSize * 0.5f, -height_ * CellSize * 0.5f};
    bounds_ = {origin.x, origin.y, width_ * CellSize, height_ * CellSize};
    bool foundEntry = false;
    bool foundExit = false;
    for (int y = 0; y < height_; ++y) {
        const std::string& row = definition.layout[static_cast<std::size_t>(y)];
        for (int x = 0; x < static_cast<int>(row.size()); ++x) {
            const char cell = row[static_cast<std::size_t>(x)];
            cells_[static_cast<std::size_t>(CellIndex(x, y))] = cell;
            if (cell == '#' || cell == 'S' || cell == 'E' || cell == 'X') ++walkableCount_;
            if (cell == 'S') spawnPoints_.push_back(CellCenter(x, y));
            if (cell == 'E') { entry_ = CellCenter(x, y); foundEntry = true; }
            if (cell == 'X') { exit_ = CellCenter(x, y); foundExit = true; }
        }
    }
    flowDistance_.assign(cells_.size(), -1);
    flowTarget_ = -1;
    exitOpen_ = false;
    return foundEntry && foundExit && !spawnPoints_.empty();
}

bool ExpeditionMap::CellWalkable(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return false;
    const char cell = cells_[static_cast<std::size_t>(CellIndex(x, y))];
    return cell == '#' || cell == 'S' || cell == 'E' || cell == 'X';
}

void ExpeditionMap::WorldToCell(Vector2 position, int& x, int& y) const {
    x = static_cast<int>(std::floor((position.x - bounds_.x) / CellSize));
    y = static_cast<int>(std::floor((position.y - bounds_.y) / CellSize));
}

Vector2 ExpeditionMap::CellCenter(int x, int y) const {
    return {bounds_.x + (x + 0.5f) * CellSize, bounds_.y + (y + 0.5f) * CellSize};
}

bool ExpeditionMap::IsWalkable(Vector2 position, float radius) const {
    constexpr int Samples = 12;
    int x = 0, y = 0;
    WorldToCell(position, x, y);
    if (!CellWalkable(x, y)) return false;
    if (radius <= 0.1f) return true;
    for (int sample = 0; sample < Samples; ++sample) {
        const float angle = sample * (2.0f * PI / Samples);
        WorldToCell({position.x + std::cos(angle) * radius,
                     position.y + std::sin(angle) * radius}, x, y);
        if (!CellWalkable(x, y)) return false;
    }
    return true;
}

Vector2 ExpeditionMap::ConstrainMovement(Vector2 from, Vector2 desired, float radius) const {
    if (IsWalkable(desired, radius)) return desired;
    const Vector2 slideX{desired.x, from.y};
    if (IsWalkable(slideX, radius)) return slideX;
    const Vector2 slideY{from.x, desired.y};
    if (IsWalkable(slideY, radius)) return slideY;
    return IsWalkable(from, radius) ? from : FindNearestWalkablePosition(from, radius);
}

Vector2 ExpeditionMap::FindNearestWalkablePosition(Vector2 position, float radius) const {
    int startX = 0, startY = 0;
    WorldToCell(position, startX, startY);
    const int maximum = std::max(width_, height_);
    for (int ring = 0; ring <= maximum; ++ring) {
        for (int y = startY - ring; y <= startY + ring; ++y)
            for (int x = startX - ring; x <= startX + ring; ++x) {
                if (ring > 0 && x != startX - ring && x != startX + ring &&
                    y != startY - ring && y != startY + ring) continue;
                if (!CellWalkable(x, y)) continue;
                const Vector2 center = CellCenter(x, y);
                if (IsWalkable(center, radius)) return center;
            }
    }
    return entry_;
}

void ExpeditionMap::RebuildFlow(Vector2 target) const {
    int targetX = 0, targetY = 0;
    WorldToCell(FindNearestWalkablePosition(target), targetX, targetY);
    const int targetIndex = CellIndex(targetX, targetY);
    if (flowTarget_ == targetIndex) return;
    flowTarget_ = targetIndex;
    std::fill(flowDistance_.begin(), flowDistance_.end(), -1);
    std::queue<int> pending;
    flowDistance_[static_cast<std::size_t>(targetIndex)] = 0;
    pending.push(targetIndex);
    constexpr int DX[4]{1, -1, 0, 0};
    constexpr int DY[4]{0, 0, 1, -1};
    while (!pending.empty()) {
        const int index = pending.front(); pending.pop();
        const int x = index % width_;
        const int y = index / width_;
        for (int direction = 0; direction < 4; ++direction) {
            const int nx = x + DX[direction], ny = y + DY[direction];
            if (!CellWalkable(nx, ny)) continue;
            const int next = CellIndex(nx, ny);
            if (flowDistance_[static_cast<std::size_t>(next)] >= 0) continue;
            flowDistance_[static_cast<std::size_t>(next)] =
                flowDistance_[static_cast<std::size_t>(index)] + 1;
            pending.push(next);
        }
    }
}

Vector2 ExpeditionMap::DirectionToward(Vector2 from, Vector2 target) const {
    RebuildFlow(target);
    int x = 0, y = 0;
    WorldToCell(from, x, y);
    if (!CellWalkable(x, y)) return Normalize({entry_.x - from.x, entry_.y - from.y});
    const int current = flowDistance_[static_cast<std::size_t>(CellIndex(x, y))];
    if (current <= 1) return Normalize({target.x - from.x, target.y - from.y});
    int bestX = x, bestY = y, bestDistance = current;
    constexpr int DX[4]{1, -1, 0, 0};
    constexpr int DY[4]{0, 0, 1, -1};
    for (int direction = 0; direction < 4; ++direction) {
        const int nx = x + DX[direction], ny = y + DY[direction];
        if (!CellWalkable(nx, ny)) continue;
        const int distance = flowDistance_[static_cast<std::size_t>(CellIndex(nx, ny))];
        if (distance >= 0 && distance < bestDistance) {
            bestDistance = distance; bestX = nx; bestY = ny;
        }
    }
    const Vector2 waypoint = CellCenter(bestX, bestY);
    return Normalize({waypoint.x - from.x, waypoint.y - from.y});
}

Vector2 ExpeditionMap::ConstrainCamera(Vector2 target, Vector2 half) const {
    const float minX = bounds_.x + half.x;
    const float maxX = bounds_.x + bounds_.width - half.x;
    const float minY = bounds_.y + half.y;
    const float maxY = bounds_.y + bounds_.height - half.y;
    target.x = minX <= maxX ? std::clamp(target.x, minX, maxX) : bounds_.x + bounds_.width * 0.5f;
    target.y = minY <= maxY ? std::clamp(target.y, minY, maxY) : bounds_.y + bounds_.height * 0.5f;
    return target;
}

bool ExpeditionMap::Validate(std::string* error) const {
    if (width_ <= 0 || height_ <= 0 || walkableCount_ <= 0) {
        if (error) *error = "map has no walkable cells"; return false;
    }
    if (!IsWalkable(entry_) || !IsWalkable(exit_)) {
        if (error) *error = "entry or exit is not walkable"; return false;
    }
    RebuildFlow(exit_);
    int entryX = 0, entryY = 0;
    WorldToCell(entry_, entryX, entryY);
    if (flowDistance_[static_cast<std::size_t>(CellIndex(entryX, entryY))] < 0) {
        if (error) *error = "exit is unreachable from entry"; return false;
    }
    for (Vector2 spawn : spawnPoints_) {
        int x = 0, y = 0; WorldToCell(spawn, x, y);
        if (!IsWalkable(spawn) || flowDistance_[static_cast<std::size_t>(CellIndex(x, y))] < 0) {
            if (error) *error = "spawn is invalid or disconnected"; return false;
        }
    }
    return true;
}

void ExpeditionMap::DrawBackground() const {
    const float time = static_cast<float>(GetTime());
    DrawRectangleRec({bounds_.x - 900.0f, bounds_.y - 700.0f,
                      bounds_.width + 1800.0f, bounds_.height + 1400.0f}, Color{4, 5, 13, 255});
    for (int y = 0; y < height_; ++y) for (int x = 0; x < width_; ++x) {
        const char cell = cells_[static_cast<std::size_t>(CellIndex(x, y))];
        const Vector2 center = CellCenter(x, y);
        if (CellWalkable(x, y)) {
            const unsigned int hash = CellHash(x, y);
            const Color ground{static_cast<unsigned char>(29 + hash % 8),
                               static_cast<unsigned char>(34 + (hash >> 3u) % 8),
                               static_cast<unsigned char>(49 + (hash >> 6u) % 11), 255};
            DrawRectangleRec({center.x - CellSize * 0.51f, center.y - CellSize * 0.51f,
                              CellSize * 1.02f, CellSize * 1.02f}, ground);
            const int cellX = static_cast<int>(center.x - 32.0f);
            const int cellY = static_cast<int>(center.y - 32.0f);
            if (!CellWalkable(x - 1, y)) DrawRectangleGradientH(cellX, cellY, 13, 64, Color{6, 8, 17, 245}, ground);
            if (!CellWalkable(x + 1, y)) DrawRectangleGradientH(cellX + 51, cellY, 13, 64, ground, Color{6, 8, 17, 245});
            if (!CellWalkable(x, y - 1)) DrawRectangleGradientV(cellX, cellY, 64, 13, Color{6, 8, 17, 245}, ground);
            if (!CellWalkable(x, y + 1)) DrawRectangleGradientV(cellX, cellY + 51, 64, 13, ground, Color{6, 8, 17, 245});
            if (hash % 7u == 0u)
                DrawLineEx({center.x - 13, center.y + 5}, {center.x + 11, center.y - 8}, 2.0f, Color{82, 75, 112, 105});
            if (hash % 11u == 0u)
                DrawPolyLinesEx({center.x + 8, center.y + 10}, 6, 9.0f, time * 5.0f + hash % 90u,
                                1.5f, Color{75, 180, 175, 75});
        } else if (cell == 'O') {
            DrawCircleV({center.x + 5, center.y + 8}, 27.0f, Color{2, 3, 8, 230});
            DrawPoly(center, 6, 25.0f, static_cast<float>(CellHash(x, y) % 60u), Color{40, 39, 57, 255});
            DrawPolyLinesEx(center, 6, 20.0f, 0.0f, 3.0f, Color{82, 72, 108, 190});
        } else if (CellHash(x, y) % 17u == 0u) {
            DrawCircleV(center, 3.0f + std::sin(time + x) * 1.0f, Color{80, 55, 135, 55});
        }
    }
    const float pulse = 21.0f + std::sin(time * 3.0f) * 4.0f;
    DrawRing(exit_, pulse, pulse + 5.0f, 0, 360, 28,
             exitOpen_ ? Color{105, 245, 215, 235} : Color{85, 80, 110, 100});
    DrawCircleV(entry_, 10.0f, Color{85, 180, 225, 90});
}

void ExpeditionMap::DrawForeground(Vector2 playerPosition) const {
    (void)playerPosition;
}

void ExpeditionMap::DrawDebug() const {
    for (int y = 0; y < height_; ++y) for (int x = 0; x < width_; ++x) {
        const char cell = cells_[static_cast<std::size_t>(CellIndex(x, y))];
        const Vector2 center = CellCenter(x, y);
        Color color = CellWalkable(x, y) ? Color{60, 220, 130, 55} : Color{220, 55, 80, 35};
        if (cell == 'S') color = Color{255, 175, 55, 115};
        if (cell == 'E') color = Color{70, 170, 255, 150};
        if (cell == 'X') color = Color{190, 90, 255, 150};
        DrawRectangleRec({center.x - 30, center.y - 30, 60, 60}, color);
    }
    DrawRectangleLinesEx(bounds_, 4.0f, YELLOW);
}
