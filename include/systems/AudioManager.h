#pragma once

#include <array>

#include "raylib.h"

enum class AudioCue {
    UiHover, UiClick, ArcBolt, Flame, Orbital, Spectral, VoidLance, Thunder,
    PlayerHit, EnemyHit, EnemyDeath, XPCollect, LevelUp, Pickup, BossSpawn,
    BossAttack, Chest, Evolution, GameOver, Victory, Count
};

enum class MusicTrack { None, Menu, Gameplay, Boss, Victory };

class AudioManager {
public:
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Play(AudioCue cue);
    void RequestMusic(MusicTrack track);

    void SetMasterVolume(float value);
    void SetMusicVolume(float value) { musicVolume_ = Clamp(value); }
    void SetSfxVolume(float value) { sfxVolume_ = Clamp(value); }
    float MasterVolume() const { return masterVolume_; }
    float MusicVolume() const { return musicVolume_; }
    float SfxVolume() const { return sfxVolume_; }
    MusicTrack CurrentMusic() const { return currentTrack_; }
    bool IsReady() const { return initialized_; }

private:
    static float Clamp(float value);
    Sound CreateTone(float frequency, float duration, float brightness) const;
    Sound CreateMusicLoop(float rootFrequency, bool tense) const;
    std::size_t TrackIndex(MusicTrack track) const;

    std::array<Sound, static_cast<std::size_t>(AudioCue::Count)> sounds_{};
    std::array<Sound, 4> music_{};
    std::array<float, static_cast<std::size_t>(AudioCue::Count)> cooldowns_{};
    std::array<unsigned int, static_cast<std::size_t>(AudioCue::Count)> playCounters_{};
    float masterVolume_ = 0.8f;
    float musicVolume_ = 0.45f;
    float sfxVolume_ = 0.75f;
    float fade_ = 0.0f;
    MusicTrack currentTrack_ = MusicTrack::None;
    MusicTrack requestedTrack_ = MusicTrack::None;
    bool initialized_ = false;
};
