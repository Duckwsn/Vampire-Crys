#include "core/Application.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>

#include "gameplay/Balance.h"
#include "game_modes/WorldNavigation.h"
#include "ui/GameUI.h"
#include "ui/Localization.h"
#include "ui/VisualStyle.h"

Application::Application() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(Balance::ScreenWidth, Balance::ScreenHeight, "Vampire Crys - Geometric Survivors");
#if defined(GAME_DEBUG)
    SetRandomSeed(12345u);
#else
    SetRandomSeed(static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
#endif
    settings_.Load((std::filesystem::path(GetApplicationDirectory()) / "settings.json").string());
    Localization::SetLanguage(settings_.language);
    InitAudioDevice();
    audio_.Initialize();
    session_.SetAudioManager(&audio_);
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    camera_.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera_.target = {0.0f, 0.0f};
    camera_.rotation = 0.0f;
    camera_.zoom = 1.0f;
}

Application::~Application() {
    settings_.fullscreen = IsWindowFullscreen();
    settings_.Save((std::filesystem::path(GetApplicationDirectory()) / "settings.json").string());
    audio_.Shutdown();
    CloseAudioDevice();
    CloseWindow();
}

void Application::Run() {
    while (!shouldQuit_ && !WindowShouldClose()) {
        const float deltaTime = std::min(GetFrameTime(), 0.05f);
        Update(deltaTime);
        Draw();
    }
}

void Application::StartNewRun(GameModeType mode, CharacterId character, SurvivalRunConfig config,
                              ExpeditionRunConfig expeditionConfig) {
    if (!gameModes_.ActivateMode(mode, session_, character, config, expeditionConfig)) return;
    state_ = GameState::Playing;
    cameraFollow_ = session_.GetPlayer().Position();
    bossActiveLastFrame_ = false;
    audio_.Play(AudioCue::UiClick);
    UpdateCamera();
}

void Application::RestartCurrentRun() {
    const GameModeType mode = gameModes_.ActiveType();
    const CharacterId character = gameModes_.Context().character;
    StartNewRun(mode, character, gameModes_.Context().survivalConfig,
                gameModes_.Context().expeditionConfig);
}

void Application::ReturnToMainMenu() {
    gameModes_.ExitActiveMode(session_);
    state_ = GameState::MainMenu;
}

void Application::Update(float deltaTime) {
#if defined(GAME_DEBUG)
    if (IsKeyPressed(KEY_F3)) {
        debugVisible_ = !debugVisible_;
    }
    if (IsKeyPressed(KEY_F2)) collisionDebugVisible_ = !collisionDebugVisible_;
    if (debugVisible_ && state_ == GameState::SurvivalSetup) {
        if (survivalSetup_.challenge == ChallengeId::None) {
            if (IsKeyPressed(KEY_D)) {
                const int next = (static_cast<int>(survivalSetup_.difficulty) + 1) %
                                 static_cast<int>(DifficultyTier::Count);
                survivalSetup_.difficulty = static_cast<DifficultyTier>(next);
            }
            if (IsKeyPressed(KEY_A)) survivalSetup_.ascension = (survivalSetup_.ascension + 1) % 11;
            if (IsKeyPressed(KEY_L)) survivalSetup_.endless = !survivalSetup_.endless;
            if (IsKeyPressed(KEY_U)) {
                survivalSetup_.ToggleMutator(static_cast<MutatorId>(debugStage10Mutator_));
                debugStage10Mutator_ = (debugStage10Mutator_ + 1) % static_cast<int>(MutatorId::Count);
            }
        }
        if (IsKeyPressed(KEY_J)) {
            const int next = (static_cast<int>(survivalSetup_.challenge) + 1) %
                             static_cast<int>(ChallengeId::Count);
            if (next == 0) survivalSetup_ = {};
            else {
                survivalSetup_.challenge = static_cast<ChallengeId>(next);
                survivalSetup_ = ResolveSurvivalConfig(survivalSetup_);
            }
        }
    }
#endif

    MusicTrack track = MusicTrack::Menu;
    if (state_ == GameState::Playing || state_ == GameState::Paused ||
        state_ == GameState::LevelUp || state_ == GameState::ChestReward)
        track = session_.Bosses().IsActive() ? MusicTrack::Boss : MusicTrack::Gameplay;
    else if (state_ == GameState::Victory) track = MusicTrack::Victory;
    else if (state_ == GameState::GameOver) track = MusicTrack::None;
    audio_.RequestMusic(track);
    ApplySettings();
    audio_.Update(deltaTime);
    if (state_ != GameState::Playing || !IsWindowFocused()) suppressAttacksUntilReleased_ = true;

    // Escape is sampled once and resolved before the frame's state-specific update.
    // Returning after a transition prevents the destination state from seeing the same press.
    if (IsKeyPressed(KEY_ESCAPE) && HandleEscapePressed()) return;

    if (ShouldUpdateGameplay(state_)) {
#if defined(GAME_DEBUG)
        if (debugVisible_) {
            if (IsKeyPressed(KEY_F4)) session_.GrantDebugXP(100);
            if (IsKeyPressed(KEY_F5)) session_.GetPlayer().HealToFull();
            if (IsKeyPressed(KEY_F6)) session_.GetPlayer().ToggleDebugInvulnerability();
            if (IsKeyPressed(KEY_F7)) gameModes_.DebugSpawnEnemies(100, session_);
            if (IsKeyPressed(KEY_F8)) gameModes_.DebugSpawnEnemies(300, session_);
            if (IsKeyPressed(KEY_F9)) session_.DebugKillAll();
            if (IsKeyPressed(KEY_F10)) gameModes_.DebugSkipTime(60.0f, session_);
            if (IsKeyPressed(KEY_Y)) gameModes_.DebugSkipTime(300.0f, session_);
            if (IsKeyPressed(KEY_F11)) session_.DebugSpawnBoss(BossType::FlameWyrm);
            if (IsKeyPressed(KEY_F12)) session_.DebugSpawnBoss(BossType::VoidHerald);
            if (IsKeyPressed(KEY_B)) session_.DebugSpawnBoss(BossType::VoidHeraldAscended);
            if (IsKeyPressed(KEY_K)) session_.DebugDamageBoss(0.10f);
            if (IsKeyPressed(KEY_E)) session_.DebugPrepareEvolution(WeaponType::ArcBolt);
            if (IsKeyPressed(KEY_G)) {
                const WeaponType type = static_cast<WeaponType>(
                    static_cast<int>(WeaponType::SoulScythe) + debugStage9Weapon_);
                session_.DebugGrantWeapon(type);
                debugStage9Weapon_ = (debugStage9Weapon_ + 1) % 4;
            }
            if (IsKeyPressed(KEY_H)) {
                const int previous = (debugStage9Weapon_ + 3) % 4;
                session_.DebugPrepareEvolution(static_cast<WeaponType>(
                    static_cast<int>(WeaponType::SoulScythe) + previous));
            }
            if (IsKeyPressed(KEY_N)) gameModes_.DebugForceEvent(session_);
            if (IsKeyPressed(KEY_C)) gameModes_.DebugEndEvent();
            if (IsKeyPressed(KEY_M)) gameModes_.DebugForceSpecialWave();
            if (IsKeyPressed(KEY_V)) gameModes_.DebugSpawnMiniboss(session_, false);
            if (IsKeyPressed(KEY_X)) gameModes_.DebugSpawnMiniboss(session_, true);
            if (IsKeyPressed(KEY_I)) gameModes_.DebugCompleteStage(session_);
            if (IsKeyPressed(KEY_O)) gameModes_.DebugOpenReward(session_);
            if (IsKeyPressed(KEY_P)) gameModes_.DebugGoToBoss(session_);
        }
#endif

        if (gameModes_.ActiveType() == GameModeType::Expedition) {
            if (IsKeyPressed(KEY_Q)) session_.Loadout().SwapWeaponSlots();
            Vector2 mouseScreen = GetMousePosition();
            mouseScreen.x = std::clamp(mouseScreen.x, 0.0f, static_cast<float>(GetScreenWidth()));
            mouseScreen.y = std::clamp(mouseScreen.y, 0.0f, static_cast<float>(GetScreenHeight()));
            Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera_);
            if (session_.Navigation() != nullptr)
                mouseWorld = session_.Navigation()->FindNearestWalkablePosition(mouseWorld, 4.0f);
            const Vector2 playerPosition = session_.GetPlayer().Position();
            Vector2 aim{mouseWorld.x - playerPosition.x, mouseWorld.y - playerPosition.y};
            const float lengthSquared = aim.x * aim.x + aim.y * aim.y;
            if (lengthSquared <= 0.0001f) aim = session_.GetPlayer().Facing();
            else {
                const float inverse = 1.0f / std::sqrt(lengthSquared);
                aim = {aim.x * inverse, aim.y * inverse};
            }
            if (suppressAttacksUntilReleased_ && !IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
                !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) suppressAttacksUntilReleased_ = false;
            const bool inputAllowed = !suppressAttacksUntilReleased_ && IsWindowFocused();
            session_.SetAttackRequest({aim, mouseWorld,
                                       inputAllowed && IsMouseButtonDown(MOUSE_BUTTON_LEFT),
                                       inputAllowed && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)});
        }

        const double updateStart = GetTime();
        const RunOutcome outcome = gameModes_.UpdateActiveMode(deltaTime, session_);
        if (session_.Bosses().IsActive() && !bossActiveLastFrame_) audio_.Play(AudioCue::BossSpawn);
        bossActiveLastFrame_ = session_.Bosses().IsActive();
        session_.SetUpdateMilliseconds(static_cast<float>((GetTime() - updateStart) * 1000.0));
        UpdateCamera();
        if (outcome == RunOutcome::Defeat) {
            session_.ClearBossHazards();
            audio_.Play(AudioCue::GameOver);
            state_ = GameState::GameOver;
        } else if (outcome == RunOutcome::Victory) {
            audio_.Play(AudioCue::Victory);
            state_ = GameState::Victory;
        } else if (session_.HasPendingLevelUp()) {
            session_.PrepareUpgradeChoices();
            if (session_.HasPendingLevelUp()) state_ = GameState::LevelUp;
        } else if (session_.HasPendingChestReward()) {
            state_ = GameState::ChestReward;
        } else if (gameModes_.HUDData().routeChoiceActive) {
            state_ = GameState::ExpeditionRoute;
        }
    }
}

void Application::Draw() {
    const double drawStart = GetTime();
    BeginDrawing();
    ClearBackground(Color{13, 15, 20, 255});

    if (state_ == GameState::MainMenu) {
        const GameUI::MenuAction action = GameUI::DrawMainMenu();
        if (action == GameUI::MenuAction::Start) {
            audio_.Play(AudioCue::UiClick);
            state_ = GameState::ModeSelection;
        } else if (action == GameUI::MenuAction::Settings) {
            audio_.Play(AudioCue::UiClick);
            settingsReturnState_ = GameState::MainMenu;
            state_ = GameState::Settings;
        } else if (action == GameUI::MenuAction::Quit) {
            shouldQuit_ = true;
        }
    } else if (state_ == GameState::ModeSelection) {
        const GameUI::MenuAction action = GameUI::DrawModeSelection();
        if (action == GameUI::MenuAction::PlaySurvival) {
            audio_.Play(AudioCue::UiClick);
            pendingMode_ = GameModeType::Survival;
            state_ = GameState::CharacterSelection;
        } else if (action == GameUI::MenuAction::PlayExpedition) {
            audio_.Play(AudioCue::UiClick);
            pendingMode_ = GameModeType::Expedition;
            state_ = GameState::CharacterSelection;
        } else if (action == GameUI::MenuAction::Back) {
            audio_.Play(AudioCue::UiClick);
            state_ = GameState::MainMenu;
        }
    } else if (state_ == GameState::CharacterSelection) {
        const int selection = GameUI::DrawCharacterSelection();
        if (selection >= 0) {
            pendingCharacter_ = static_cast<CharacterId>(selection);
            audio_.Play(AudioCue::UiClick);
            if (pendingMode_ == GameModeType::Survival) state_ = GameState::SurvivalSetup;
            else StartNewRun(GameModeType::Expedition, pendingCharacter_);
        }
        else if (selection == -2) { audio_.Play(AudioCue::UiClick); state_ = GameState::ModeSelection; }
    } else if (state_ == GameState::SurvivalSetup) {
        const GameUI::SetupAction action = GameUI::DrawSurvivalSetup(pendingCharacter_, survivalSetup_);
        if (action == GameUI::SetupAction::QuickStart) {
            survivalSetup_ = {};
            StartNewRun(GameModeType::Survival, pendingCharacter_, survivalSetup_);
        } else if (action == GameUI::SetupAction::Start) {
            StartNewRun(GameModeType::Survival, pendingCharacter_, survivalSetup_);
        } else if (action == GameUI::SetupAction::Back) {
            audio_.Play(AudioCue::UiClick); state_ = GameState::CharacterSelection;
        }
    } else if (state_ == GameState::Settings) {
        const GameUI::MenuAction action = GameUI::DrawSettings(settings_);
        if (action == GameUI::MenuAction::Back) {
            audio_.Play(AudioCue::UiClick);
            settings_.Save((std::filesystem::path(GetApplicationDirectory()) / "settings.json").string());
            state_ = settingsReturnState_;
        }
    } else {
        BeginMode2D(camera_);
        session_.DrawWorld();
        if (collisionDebugVisible_ || (debugVisible_ && gameModes_.ActiveType() == GameModeType::Expedition))
            session_.DrawCollisionDebug();
        EndMode2D();
        const ModeHUDData modeHUD = gameModes_.HUDData();
        if (modeHUD.eventActive && std::strcmp(modeHUD.eventName, "BLOOD MOON") == 0)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{125, 12, 28, 34});
        if (state_ == GameState::Playing || state_ == GameState::Paused) {
            VisualStyle::DrawViewportFade();
        }
        GameUI::DrawHUD(session_, modeHUD);
        if (state_ == GameState::Playing && modeHUD.mode == GameModeType::Expedition) {
            Vector2 cursor = GetMousePosition();
            cursor.x = std::clamp(cursor.x, 9.0f, static_cast<float>(GetScreenWidth()) - 9.0f);
            cursor.y = std::clamp(cursor.y, 9.0f, static_cast<float>(GetScreenHeight()) - 9.0f);
            const Color crosshair{145, 225, 255, 235};
            DrawRectangleLinesEx({cursor.x - 5.0f, cursor.y - 5.0f, 10.0f, 10.0f}, 2.0f, crosshair);
            DrawLineEx({cursor.x - 11.0f, cursor.y}, {cursor.x - 7.0f, cursor.y}, 2.0f, crosshair);
            DrawLineEx({cursor.x + 7.0f, cursor.y}, {cursor.x + 11.0f, cursor.y}, 2.0f, crosshair);
            DrawLineEx({cursor.x, cursor.y - 11.0f}, {cursor.x, cursor.y - 7.0f}, 2.0f, crosshair);
            DrawLineEx({cursor.x, cursor.y + 7.0f}, {cursor.x, cursor.y + 11.0f}, 2.0f, crosshair);
        }
        if (modeHUD.stageIntroRemaining > 0.0f) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{5, 6, 15, 105});
            const char* stage = TextFormat(Localization::Text("STAGE %d", "FASE %d"), modeHUD.progressIndex);
            DrawText(stage, (GetScreenWidth() - MeasureText(stage, 28)) / 2, GetScreenHeight() / 2 - 48, 28, GOLD);
            DrawText(Localization::Known(modeHUD.progressName),
                     (GetScreenWidth() - MeasureText(Localization::Known(modeHUD.progressName), 42)) / 2,
                     GetScreenHeight() / 2 - 8, 42, RAYWHITE);
        }
        if (modeHUD.transitionAlpha > 0.0f)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, modeHUD.transitionAlpha));

        if (state_ == GameState::Paused) {
            const GameUI::MenuAction action = GameUI::DrawPause();
            if (action == GameUI::MenuAction::Resume) {
                audio_.Play(AudioCue::UiClick); state_ = GameState::Playing;
            } else if (action == GameUI::MenuAction::Settings) {
                audio_.Play(AudioCue::UiClick); settingsReturnState_ = GameState::Paused;
                state_ = GameState::Settings;
            } else if (action == GameUI::MenuAction::Restart) RestartCurrentRun();
            else if (action == GameUI::MenuAction::MainMenu) {
                audio_.Play(AudioCue::UiClick); ReturnToMainMenu();
            } else if (action == GameUI::MenuAction::Quit) shouldQuit_ = true;
        } else if (state_ == GameState::LevelUp) {
            const int selection = GameUI::DrawLevelUp(session_);
            if (selection >= 0) {
                audio_.Play(AudioCue::UiClick);
                session_.SelectUpgrade(selection);
                if (session_.HasPendingWeaponReplacement()) {
                    state_ = GameState::WeaponReplacement;
                } else if (session_.HasPendingLevelUp()) {
                    session_.PrepareUpgradeChoices();
                } else {
                    state_ = session_.HasPendingChestReward() ? GameState::ChestReward
                                                              : GameState::Playing;
                }
            }
        } else if (state_ == GameState::ChestReward) {
            const int selection = GameUI::DrawChestReward(session_, gameModes_.ActiveType());
            if (selection >= 0) {
                audio_.Play(AudioCue::UiClick);
                session_.SelectChestReward(selection);
                if (session_.HasPendingWeaponReplacement()) {
                    state_ = GameState::WeaponReplacement;
                } else if (session_.HasPendingLevelUp()) {
                    session_.PrepareUpgradeChoices();
                    state_ = session_.HasPendingLevelUp() ? GameState::LevelUp : GameState::Playing;
                } else {
                    state_ = GameState::Playing;
                }
            }
        } else if (state_ == GameState::WeaponReplacement) {
            const int selection = GameUI::DrawWeaponReplacement(session_);
            if (selection >= 0 && session_.ConfirmWeaponReplacement(selection)) {
                audio_.Play(AudioCue::UiClick);
                if (session_.HasPendingLevelUp()) {
                    session_.PrepareUpgradeChoices();
                    state_ = GameState::LevelUp;
                } else state_ = session_.HasPendingChestReward() ? GameState::ChestReward
                                                                 : GameState::Playing;
            } else if (selection == -2) {
                const bool fromLevelUp = session_.ReplacementFromLevelUp();
                session_.CancelWeaponReplacement();
                audio_.Play(AudioCue::UiClick);
                state_ = fromLevelUp ? GameState::LevelUp : GameState::ChestReward;
            }
        } else if (state_ == GameState::ExpeditionRoute) {
            const int selection = GameUI::DrawExpeditionRoute(gameModes_.ExpeditionRoute());
            if (selection >= 0 && gameModes_.SelectRoute(selection, session_)) {
                audio_.Play(AudioCue::UiClick);
                state_ = session_.HasPendingChestReward() ? GameState::ChestReward
                                                          : GameState::Playing;
                cameraFollow_ = session_.GetPlayer().Position();
                UpdateCamera();
            }
        } else if (state_ == GameState::GameOver) {
            const GameUI::MenuAction action = GameUI::DrawGameOver(session_, modeHUD);
            if (action == GameUI::MenuAction::Restart) {
                RestartCurrentRun();
            } else if (action == GameUI::MenuAction::MainMenu) {
                ReturnToMainMenu();
            }
        } else if (state_ == GameState::Victory) {
            const GameUI::MenuAction action = GameUI::DrawVictory(session_, modeHUD);
            if (action == GameUI::MenuAction::Restart) RestartCurrentRun();
            else if (action == GameUI::MenuAction::MainMenu) ReturnToMainMenu();
        }

        if (debugVisible_) {
            GameUI::DrawDebugOverlay(session_, modeHUD);
        }
    }

    session_.SetDrawMilliseconds(static_cast<float>((GetTime() - drawStart) * 1000.0));
    EndDrawing();
}

void Application::UpdateCamera() {
    camera_.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    const Vector2 player = session_.GetPlayer().Position();
    const float blend = std::min(1.0f, GetFrameTime() * 10.0f);
    cameraFollow_.x += (player.x - cameraFollow_.x) * blend;
    cameraFollow_.y += (player.y - cameraFollow_.y) * blend;
    const Vector2 shake = session_.ScreenShake().Offset(settings_.screenShake);
    camera_.target = session_.ConstrainCamera(
        {cameraFollow_.x + shake.x, cameraFollow_.y + shake.y},
        {GetScreenWidth() * 0.5f / camera_.zoom, GetScreenHeight() * 0.5f / camera_.zoom});
}

void Application::ApplySettings() {
    Localization::SetLanguage(settings_.language);
    audio_.SetMasterVolume(settings_.masterVolume);
    audio_.SetMusicVolume(settings_.musicVolume);
    audio_.SetSfxVolume(settings_.sfxVolume);
    if (settings_.fullscreen != IsWindowFullscreen()) ToggleFullscreen();
}

bool Application::HandleEscapePressed() {
    switch (EscapeActionFor(state_)) {
        case EscapeAction::Quit:
            shouldQuit_ = true;
            return true;
        case EscapeAction::Resume:
            state_ = GameState::Playing;
            break;
        case EscapeAction::OpenPause:
            state_ = GameState::Paused;
            break;
        case EscapeAction::CloseSettings:
            settings_.Save((std::filesystem::path(GetApplicationDirectory()) / "settings.json").string());
            state_ = settingsReturnState_;
            break;
        case EscapeAction::ReturnToMainMenu:
            ReturnToMainMenu();
            break;
        case EscapeAction::ReturnToModeSelection:
            state_ = GameState::ModeSelection;
            break;
        case EscapeAction::ReturnToCharacterSelection:
            state_ = GameState::CharacterSelection;
            break;
        case EscapeAction::None:
            return false;
    }
    audio_.Play(AudioCue::UiClick);
    return true;
}
