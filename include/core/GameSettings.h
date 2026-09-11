#pragma once

#include <string>

enum class GameLanguage { English, PortugueseBrazil };

struct GameSettings {
    float masterVolume = 0.80f;
    float musicVolume = 0.45f;
    float sfxVolume = 0.75f;
    float screenShake = 1.0f;
    bool fullscreen = false;
    GameLanguage language = GameLanguage::English;

    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
};
