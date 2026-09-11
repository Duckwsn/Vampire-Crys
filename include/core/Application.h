#pragma once

#include "core/GameFlow.h"
#include "gameplay/GameSession.h"
#include "game_modes/GameModeManager.h"
#include "core/GameSettings.h"
#include "systems/AudioManager.h"

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();

private:
    void StartNewRun(GameModeType mode, CharacterId character = CharacterId::Hunter,
                     SurvivalRunConfig config = {}, ExpeditionRunConfig expeditionConfig = {});
    void RestartCurrentRun();
    void ReturnToMainMenu();
    void Update(float deltaTime);
    void Draw();
    void UpdateCamera();
    void ApplySettings();
    bool HandleEscapePressed();

    GameState state_ = GameState::MainMenu;
    GameSession session_;
    GameModeManager gameModes_;
    Camera2D camera_{};
    Vector2 cameraFollow_{};
    AudioManager audio_;
    GameSettings settings_{};
    GameState settingsReturnState_ = GameState::MainMenu;
    bool debugVisible_ = false;
    bool collisionDebugVisible_ = false;
    bool bossActiveLastFrame_ = false;
    bool suppressAttacksUntilReleased_ = true;
    bool shouldQuit_ = false;
    int debugStage9Weapon_ = 0;
    int debugStage10Mutator_ = 0;
    CharacterId pendingCharacter_ = CharacterId::Hunter;
    SurvivalRunConfig survivalSetup_{};
    GameModeType pendingMode_ = GameModeType::Survival;
};
