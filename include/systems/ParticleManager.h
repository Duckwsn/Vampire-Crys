#pragma once

#include <vector>
#include "raylib.h"

enum class ParticleKind { Spark, Square, Shard, Ring };
enum class ParticlePriority { Low, Normal, Important };

struct Particle {
    Vector2 position{};
    Vector2 velocity{};
    float lifetime = 0.0f;
    float age = 0.0f;
    float size = 1.0f;
    float alpha = 1.0f;
    Color color = WHITE;
    ParticleKind kind = ParticleKind::Spark;
    ParticlePriority priority = ParticlePriority::Normal;
    bool active = false;
};

class ParticleManager {
public:
    ParticleManager();
    void Reset();
    void SpawnBurst(Vector2 position, Color color, int count, float speed = 90.0f,
                    ParticleKind kind = ParticleKind::Spark,
                    ParticlePriority priority = ParticlePriority::Normal);
    void Update(float deltaTime);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }
private:
    std::vector<Particle> particles_;
    int activeCount_ = 0;
    bool poolWarningEmitted_ = false;
};
