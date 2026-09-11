#include "systems/AudioManager.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr int SampleRate = 22050;
}

float AudioManager::Clamp(float value) { return std::clamp(value, 0.0f, 1.0f); }

Sound AudioManager::CreateTone(float frequency, float duration, float brightness) const {
    const int frameCount = std::max(1, static_cast<int>(duration * SampleRate));
    std::vector<float> samples(static_cast<std::size_t>(frameCount));
    for (int frame = 0; frame < frameCount; ++frame) {
        const float t = static_cast<float>(frame) / SampleRate;
        const float envelope = std::pow(1.0f - static_cast<float>(frame) / frameCount, 2.0f);
        const float fundamental = std::sin(2.0f * PI * frequency * t);
        const float harmonic = std::sin(2.0f * PI * frequency * 2.01f * t) * brightness;
        samples[static_cast<std::size_t>(frame)] = (fundamental + harmonic) * envelope * 0.34f;
    }
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frameCount);
    wave.sampleRate = SampleRate;
    wave.sampleSize = 32;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

Sound AudioManager::CreateMusicLoop(float rootFrequency, bool tense) const {
    constexpr float duration = 4.0f;
    const int frameCount = static_cast<int>(duration * SampleRate);
    std::vector<float> samples(static_cast<std::size_t>(frameCount));
    for (int frame = 0; frame < frameCount; ++frame) {
        const float t = static_cast<float>(frame) / SampleRate;
        const float beat = std::fmod(t * (tense ? 2.5f : 1.5f), 1.0f);
        const float pulse = std::exp(-beat * 8.0f);
        const int step = static_cast<int>(t * 2.0f) % 4;
        const float ratio[] = {1.0f, 1.1892f, 1.4983f, tense ? 1.7818f : 1.3348f};
        const float note = rootFrequency * ratio[step];
        const float drone = std::sin(2.0f * PI * rootFrequency * 0.5f * t) * 0.08f;
        const float synth = std::sin(2.0f * PI * note * t) * pulse * 0.10f;
        samples[static_cast<std::size_t>(frame)] = drone + synth;
    }
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frameCount);
    wave.sampleRate = SampleRate;
    wave.sampleSize = 32;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

void AudioManager::Initialize() {
    if (initialized_ || !IsAudioDeviceReady()) return;
    constexpr std::array<float, static_cast<std::size_t>(AudioCue::Count)> frequencies{{
        520, 620, 760, 210, 440, 590, 145, 95, 125, 320, 180, 880, 660, 740, 72, 110, 540, 920, 82, 1040}};
    for (std::size_t index = 0; index < sounds_.size(); ++index)
        sounds_[index] = CreateTone(frequencies[index], index == 14 || index == 18 || index == 19 ? 0.55f : 0.18f,
                                    index % 3 == 0 ? 0.34f : 0.18f);
    music_[0] = CreateMusicLoop(82.41f, false);
    music_[1] = CreateMusicLoop(98.00f, false);
    music_[2] = CreateMusicLoop(73.42f, true);
    music_[3] = CreateMusicLoop(130.81f, false);
    SetMasterVolume(masterVolume_);
    initialized_ = true;
}

void AudioManager::Shutdown() {
    if (!initialized_) return;
    for (Sound& sound : sounds_) UnloadSound(sound);
    for (Sound& track : music_) UnloadSound(track);
    initialized_ = false;
}

void AudioManager::SetMasterVolume(float value) {
    masterVolume_ = Clamp(value);
    if (IsAudioDeviceReady()) ::SetMasterVolume(masterVolume_);
}

std::size_t AudioManager::TrackIndex(MusicTrack track) const {
    return static_cast<std::size_t>(static_cast<int>(track) - 1);
}

void AudioManager::Play(AudioCue cue) {
    if (!initialized_ || sfxVolume_ <= 0.0f) return;
    const std::size_t index = static_cast<std::size_t>(cue);
    if (cooldowns_[index] > 0.0f) return;
    Sound& sound = sounds_[index];
    const float pitch = 0.96f + static_cast<float>(playCounters_[index]++ % 5u) * 0.02f;
    SetSoundPitch(sound, pitch);
    SetSoundVolume(sound, sfxVolume_);
    PlaySound(sound);
    cooldowns_[index] = cue == AudioCue::EnemyHit ? 0.045f :
                        (cue == AudioCue::XPCollect ? 0.06f : 0.10f);
}

void AudioManager::RequestMusic(MusicTrack track) {
    if (requestedTrack_ != track) {
        requestedTrack_ = track;
        fade_ = 1.0f;
    }
}

void AudioManager::Update(float deltaTime) {
    if (!initialized_) return;
    for (float& cooldown : cooldowns_) cooldown = std::max(0.0f, cooldown - deltaTime);
    if (currentTrack_ != requestedTrack_) {
        fade_ = std::max(0.0f, fade_ - deltaTime * 2.5f);
        if (currentTrack_ != MusicTrack::None)
            SetSoundVolume(music_[TrackIndex(currentTrack_)], musicVolume_ * fade_);
        if (fade_ <= 0.0f) {
            if (currentTrack_ != MusicTrack::None) StopSound(music_[TrackIndex(currentTrack_)]);
            currentTrack_ = requestedTrack_;
            fade_ = 0.0f;
        }
    } else {
        fade_ = std::min(1.0f, fade_ + deltaTime * 2.0f);
    }
    if (currentTrack_ != MusicTrack::None) {
        Sound& track = music_[TrackIndex(currentTrack_)];
        SetSoundVolume(track, musicVolume_ * fade_);
        if (!IsSoundPlaying(track)) PlaySound(track);
    }
}
