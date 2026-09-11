#pragma once

#include "raylib.h"
#include "core/GameSettings.h"
#include "game_modes/GameModeTypes.h"

class GameSession;
class ExpeditionDirector;

namespace GameUI {
enum class MenuAction {
    None,
    Start,
    PlaySurvival,
    PlayExpedition,
    Resume,
    Settings,
    Back,
    Quit,
    Restart,
    MainMenu
};

MenuAction DrawMainMenu();
MenuAction DrawModeSelection();
int DrawCharacterSelection();
enum class SetupAction { None, QuickStart, Start, Back };
SetupAction DrawSurvivalSetup(CharacterId character, SurvivalRunConfig& config);
MenuAction DrawPause();
MenuAction DrawSettings(GameSettings& settings);
void DrawHUD(const GameSession& session, const ModeHUDData& mode);
int DrawLevelUp(const GameSession& session);
int DrawChestReward(const GameSession& session, GameModeType mode);
int DrawWeaponReplacement(const GameSession& session);
int DrawExpeditionRoute(const ExpeditionDirector& route);
MenuAction DrawGameOver(const GameSession& session, const ModeHUDData& mode);
MenuAction DrawVictory(const GameSession& session, const ModeHUDData& mode);
void DrawDebugOverlay(const GameSession& session, const ModeHUDData& mode);
} // namespace GameUI
