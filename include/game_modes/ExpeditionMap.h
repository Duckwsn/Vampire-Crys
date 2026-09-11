#pragma once

#include <string>
#include <vector>

#include "game_modes/WorldNavigation.h"

struct ExpeditionMapDefinition {
    const char* id = "";
    const char* name = "";
    std::vector<std::string> layout;
};

class ExpeditionMap final : public WorldNavigation {
public:
    bool Load(const ExpeditionMapDefinition& definition);
    bool Validate(std::string* error = nullptr) const;

    bool IsWalkable(Vector2 position, float radius = 0.0f) const override;
    Vector2 ConstrainMovement(Vector2 from, Vector2 desired, float radius) const override;
    Vector2 FindNearestWalkablePosition(Vector2 position, float radius = 0.0f) const override;
    Vector2 DirectionToward(Vector2 from, Vector2 target) const override;
    Vector2 ConstrainCamera(Vector2 target, Vector2 viewportHalfSize) const override;
    Rectangle Bounds() const override { return bounds_; }
    void DrawBackground() const override;
    void DrawForeground(Vector2 playerPosition) const override;
    void DrawDebug() const override;

    Vector2 Entry() const { return entry_; }
    Vector2 Exit() const { return exit_; }
    const std::vector<Vector2>& SpawnPoints() const { return spawnPoints_; }
    const char* Name() const { return name_.c_str(); }
    const char* Id() const { return id_.c_str(); }
    int Width() const { return width_; }
    int Height() const { return height_; }
    int WalkableCount() const { return walkableCount_; }
    void SetExitOpen(bool open) { exitOpen_ = open; }
    bool ExitOpen() const { return exitOpen_; }

private:
    bool CellWalkable(int x, int y) const;
    int CellIndex(int x, int y) const { return y * width_ + x; }
    void WorldToCell(Vector2 position, int& x, int& y) const;
    Vector2 CellCenter(int x, int y) const;
    void RebuildFlow(Vector2 target) const;

    static constexpr float CellSize = 64.0f;
    std::string id_;
    std::string name_;
    std::vector<char> cells_;
    std::vector<Vector2> spawnPoints_;
    Vector2 entry_{};
    Vector2 exit_{};
    Rectangle bounds_{};
    int width_ = 0;
    int height_ = 0;
    int walkableCount_ = 0;
    bool exitOpen_ = false;
    mutable std::vector<int> flowDistance_;
    mutable int flowTarget_ = -1;
};
