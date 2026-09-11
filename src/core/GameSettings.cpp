#include "core/GameSettings.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include "raylib.h"

namespace {
float Unit(float value) { return std::clamp(value, 0.0f, 1.0f); }
}

bool GameSettings::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file) return false;
    try {
        const nlohmann::json root = nlohmann::json::parse(file);
        masterVolume = Unit(root.value("master_volume", masterVolume));
        musicVolume = Unit(root.value("music_volume", musicVolume));
        sfxVolume = Unit(root.value("sfx_volume", sfxVolume));
        screenShake = Unit(root.value("screen_shake", screenShake));
        fullscreen = root.value("fullscreen", fullscreen);
        const std::string languageCode = root.value("language", std::string{"en"});
        language = languageCode == "pt-BR" ? GameLanguage::PortugueseBrazil : GameLanguage::English;
        return true;
    } catch (const std::exception& error) {
        TraceLog(LOG_WARNING, "Invalid settings.json; safe defaults retained (%s)", error.what());
        return false;
    }
}

bool GameSettings::Save(const std::string& path) const {
    const std::filesystem::path destination(path);
    const std::filesystem::path temporary = destination.string() + ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    if (!file) {
        TraceLog(LOG_WARNING, "Unable to write settings: %s", path.c_str());
        return false;
    }
    const nlohmann::json root{{"master_volume", Unit(masterVolume)},
                              {"music_volume", Unit(musicVolume)},
                              {"sfx_volume", Unit(sfxVolume)},
                              {"screen_shake", Unit(screenShake)},
                              {"fullscreen", fullscreen},
                              {"language", language == GameLanguage::PortugueseBrazil ? "pt-BR" : "en"}};
    file << root.dump(2) << '\n';
    file.close();
    if (!file) return false;
    const std::filesystem::path backup = destination.string() + ".bak";
    std::error_code error;
    std::filesystem::remove(backup, error);
    error.clear();
    const bool hadPrevious = std::filesystem::exists(destination);
    if (hadPrevious) {
        std::filesystem::rename(destination, backup, error);
        if (error) {
            TraceLog(LOG_WARNING, "Unable to protect previous settings: %s", error.message().c_str());
            std::filesystem::remove(temporary, error);
            return false;
        }
    }
    std::filesystem::rename(temporary, destination, error);
    if (error) {
        TraceLog(LOG_WARNING, "Unable to finalize settings: %s", error.message().c_str());
        std::filesystem::remove(temporary, error);
        if (hadPrevious) {
            error.clear();
            std::filesystem::rename(backup, destination, error);
        }
        return false;
    }
    if (hadPrevious) std::filesystem::remove(backup, error);
    return true;
}
