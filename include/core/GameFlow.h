#pragma once

enum class GameState {
    MainMenu,
    ModeSelection,
    CharacterSelection,
    SurvivalSetup,
    Paused,
    Settings,
    Playing,
    LevelUp,
    ChestReward,
    WeaponReplacement,
    ExpeditionRoute,
    GameOver,
    Victory
};

enum class EscapeAction {
    None,
    Quit,
    Resume,
    OpenPause,
    CloseSettings,
    ReturnToMainMenu,
    ReturnToModeSelection,
    ReturnToCharacterSelection
};

constexpr EscapeAction EscapeActionFor(GameState state) {
    switch (state) {
        case GameState::MainMenu: return EscapeAction::Quit;
        case GameState::ModeSelection: return EscapeAction::ReturnToMainMenu;
        case GameState::CharacterSelection: return EscapeAction::ReturnToModeSelection;
        case GameState::SurvivalSetup: return EscapeAction::ReturnToCharacterSelection;
        case GameState::Paused: return EscapeAction::Resume;
        case GameState::Settings: return EscapeAction::CloseSettings;
        case GameState::Playing: return EscapeAction::OpenPause;
        case GameState::GameOver:
        case GameState::Victory: return EscapeAction::ReturnToMainMenu;
        case GameState::LevelUp:
        case GameState::ChestReward: return EscapeAction::None;
        case GameState::WeaponReplacement: return EscapeAction::None;
        case GameState::ExpeditionRoute: return EscapeAction::None;
    }
    return EscapeAction::None;
}

constexpr bool ShouldUpdateGameplay(GameState state) {
    return state == GameState::Playing;
}
