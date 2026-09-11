#include "ui/GameUI.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>

#include "gameplay/Balance.h"
#include "gameplay/GameSession.h"
#include "game_modes/ExpeditionDirector.h"
#include "ui/Localization.h"
#include "ui/VisualStyle.h"

namespace {
const char* T(const char* english, const char* portuguese) {
    return Localization::Text(english, portuguese);
}

const char* K(const char* knownEnglish) { return Localization::Known(knownEnglish); }

void ReplaceAll(std::string& text, const char* from, const char* to) {
    std::size_t position = 0;
    const std::size_t fromLength = std::strlen(from);
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, fromLength, to);
        position += std::strlen(to);
    }
}

std::string LocalizedEffect(const std::string& english) {
    if (!Localization::IsPortuguese()) return english;
    if (const char* known = K(english.c_str()); known != english.c_str()) return known;
    std::string result = english;
    const std::pair<const char*, const char*> terms[]{
        {"projectile speed", "velocidade dos projeteis"}, {"explosion radius", "raio da explosao"},
        {"global cooldowns", "recargas globais"}, {"global damage", "dano global"},
        {"critical chance", "chance critica"}, {"maximum health", "vida maxima"},
        {"max HP", "HP maximo"}, {"movement speed", "velocidade de movimento"},
        {"damage tick", "pulso de dano"}, {"wider orbit", "orbita mais ampla"},
        {"orbit speed", "velocidade orbital"}, {"orb size", "tamanho do orbe"},
        {"opposite sweep", "varredura oposta"}, {"degrees arc", "graus de arco"},
        {"projectiles", "projeteis"}, {"projectile", "projetil"}, {"piercing", "perfuracao"},
        {"cooldown", "recarga"}, {"damage", "dano"}, {"duration", "duracao"},
        {"radius", "raio"}, {"speed", "velocidade"}, {"armor", "armadura"},
        {"luck", "sorte"}, {"heal", "cura"}, {"slow effectiveness", "eficacia da lentidao"},
        {"control", "controle"}, {"orbs", "orbes"}, {"orb", "orbe"},
        {"shards", "estilhacos"}, {"shard", "estilhaco"},
        {"needles", "agulhas"}, {"needle", "agulha"}, {"ticks", "pulsos"}, {"tick", "pulso"},
        {"sweep", "varredura"}, {"area", "area"}, {"below", "abaixo de"},
        {"enemies", "inimigos"}, {"enemy", "inimigo"},
    };
    for (const auto& term : terms) ReplaceAll(result, term.first, term.second);
    ReplaceAll(result, " and ", " e ");
    ReplaceAll(result, "longer ", "maior ");
    ReplaceAll(result, "wider ", "mais amplo ");
    return result;
}

bool DrawButton(Rectangle bounds, const char* label, int fontSize = 28) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRounded(bounds, 0.18f, 8, hovered ? Color{102, 76, 150, 255}
                                                    : Color{68, 55, 95, 255});
    DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 2.0f,
                                hovered ? Color{220, 196, 255, 255} : Color{130, 110, 165, 255});
    const int textWidth = MeasureText(label, fontSize);
    DrawText(label, static_cast<int>(bounds.x + (bounds.width - textWidth) * 0.5f),
             static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f), fontSize, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawBar(Rectangle bounds, float fraction, Color fill) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    DrawRectangleRec(bounds, Color{20, 21, 27, 230});
    DrawRectangleRec({bounds.x + 2.0f, bounds.y + 2.0f,
                      (bounds.width - 4.0f) * fraction, bounds.height - 4.0f}, fill);
    DrawRectangleLinesEx(bounds, 2.0f, Color{220, 220, 230, 210});
}

void DrawCentered(const char* text, int y, int fontSize, Color color) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, fontSize)) / 2, y, fontSize, color);
}

void DrawWrappedText(const char* text, Rectangle bounds, int fontSize, Color color) {
    std::string line;
    std::string word;
    float y = bounds.y;
    const auto flushWord = [&]() {
        if (word.empty()) return;
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > bounds.width) {
            DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
            y += fontSize + 2.0f;
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
    };
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor == ' ') flushWord();
        else word.push_back(*cursor);
    }
    flushWord();
    if (!line.empty() && y + fontSize <= bounds.y + bounds.height)
        DrawText(line.c_str(), static_cast<int>(bounds.x), static_cast<int>(y), fontSize, color);
}

std::string MutatorSummary(const SurvivalRunConfig& config) {
    std::string result;
    for (int raw = 0; raw < static_cast<int>(MutatorId::Count); ++raw) {
        const MutatorId id = static_cast<MutatorId>(raw);
        if (!config.HasMutator(id)) continue;
        if (!result.empty()) result += " / ";
        result += K(GetMutatorDefinition(id).name);
    }
    return result.empty() ? K("NONE") : result;
}

void DrawMenuBackdrop(bool showEmblem = true) {
    ClearBackground(Color{7, 9, 17, 255});
    const float time = static_cast<float>(GetTime());
    for (int i = 0; i < 32; ++i) {
        const float x = std::fmod(i * 173.0f + time * (9.0f + i % 4), GetScreenWidth() + 80.0f) - 40.0f;
        const float y = std::fmod(i * 97.0f + time * (5.0f + i % 3), GetScreenHeight() + 80.0f) - 40.0f;
        DrawPoly({x, y}, 4 + i % 3, 3.0f + i % 4, time * 18.0f + i * 11.0f,
                 Color{75, 105, 150, static_cast<unsigned char>(35 + i % 4 * 9)});
    }
    if (showEmblem) {
        const Vector2 center{GetScreenWidth() * 0.5f, 112.0f};
        const Color energy = VisualStyle::EnergyColor(time, 0.8f, 1.0f);
        DrawPoly(center, 4, 45.0f + std::sin(time * 2.4f) * 3.0f, 45.0f + time * 18.0f,
                 Color{energy.r, energy.g, energy.b, 45});
        DrawPolyLinesEx(center, 4, 34.0f, 45.0f - time * 25.0f, 3.0f, energy);
    }
}

void DrawSlider(const char* label, Rectangle bounds, float& value) {
    DrawText(label, static_cast<int>(bounds.x), static_cast<int>(bounds.y - 29), 19, RAYWHITE);
    DrawRectangleRounded(bounds, 0.5f, 8, Color{20, 25, 38, 255});
    DrawRectangleRounded({bounds.x, bounds.y, bounds.width * std::clamp(value, 0.0f, 1.0f), bounds.height},
                         0.5f, 8, Color{68, 214, 190, 255});
    DrawCircleV({bounds.x + bounds.width * value, bounds.y + bounds.height * 0.5f}, 11.0f, RAYWHITE);
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),
        {bounds.x - 12, bounds.y - 12, bounds.width + 24, bounds.height + 24}))
        value = std::clamp((GetMouseX() - bounds.x) / bounds.width, 0.0f, 1.0f);
    DrawText(TextFormat("%d%%", static_cast<int>(value * 100.0f)),
             static_cast<int>(bounds.x + bounds.width + 22), static_cast<int>(bounds.y - 3), 18, RAYWHITE);
}
} // namespace

GameUI::MenuAction GameUI::DrawMainMenu() {
    DrawMenuBackdrop();
    DrawCentered("VAMPIRE CRYS", 154, 58, Color{218, 245, 255, 255});
    DrawCentered(T("GEOMETRIC SURVIVORS", "SOBREVIVENTES GEOMÉTRICOS"), 220, 20, Color{120, 190, 190, 255});

    const float centerX = GetScreenWidth() * 0.5f;
    const float buttonY = std::min(330.0f, GetScreenHeight() - 255.0f);
    if (DrawButton({centerX - 130.0f, buttonY, 260.0f, 62.0f}, T("PLAY", "JOGAR")) ||
        IsKeyPressed(KEY_ENTER)) {
        return MenuAction::Start;
    }
    if (DrawButton({centerX - 130.0f, buttonY + 73.0f, 260.0f, 58.0f}, T("SETTINGS", "CONFIGURAÇÕES"), 24))
        return MenuAction::Settings;
    if (DrawButton({centerX - 130.0f, buttonY + 146.0f, 260.0f, 58.0f}, T("QUIT", "SAIR"), 24)) {
        return MenuAction::Quit;
    }
    if (GetScreenHeight() >= 620) {
#if defined(GAME_DEBUG)
    DrawCentered(T("WASD / ARROWS TO MOVE  |  F2 HITBOXES  |  F3 DEBUG",
                   "WASD / SETAS PARA MOVER  |  F2 HITBOXES  |  F3 DEBUG"), GetScreenHeight() - 52, 18,
                 Color{120, 116, 135, 255});
#else
    DrawCentered(T("WASD / ARROWS TO MOVE  |  ESC PAUSE", "WASD / SETAS PARA MOVER  |  ESC PAUSA"), GetScreenHeight() - 52, 18,
                 Color{120, 116, 135, 255});
#endif
    }
    DrawText("v" VAMPIRECRYS_VERSION, 14, GetScreenHeight() - 26, 14, Color{90, 105, 120, 255});
    return MenuAction::None;
}

GameUI::MenuAction GameUI::DrawModeSelection() {
    DrawMenuBackdrop();
    DrawCentered(T("SELECT MODE", "SELECIONE O MODO"), 52, 46, Color{218, 245, 255, 255});
    DrawCentered(T("Choose how the run will unfold", "Escolha como a partida vai se desenvolver"), 108, 19, Color{140, 180, 195, 255});

    const float gap = 34.0f;
    const float cardWidth = std::min(420.0f, (GetScreenWidth() - 100.0f - gap) * 0.5f);
    const float startX = (GetScreenWidth() - cardWidth * 2.0f - gap) * 0.5f;
    const float cardY = 158.0f;
    const float cardHeight = std::min(390.0f, static_cast<float>(GetScreenHeight()) - 238.0f);
    const Rectangle survival{startX, cardY, cardWidth, cardHeight};
    const Rectangle expedition{startX + cardWidth + gap, cardY, cardWidth, cardHeight};

    DrawRectangleRounded(survival, 0.08f, 10, Color{17, 31, 42, 245});
    DrawRectangleRoundedLinesEx(survival, 0.08f, 10, 2.5f, Color{80, 220, 185, 230});
    DrawText("SURVIVAL", static_cast<int>(survival.x + 24), static_cast<int>(survival.y + 24),
             30, Color{135, 245, 215, 255});
    DrawText(T("AVAILABLE", "DISPONÍVEL"), static_cast<int>(survival.x + 24), static_cast<int>(survival.y + 66),
             16, GREEN);
    DrawText(T("Face an increasingly dangerous horde.", "Enfrente uma horda cada vez mais perigosa."), static_cast<int>(survival.x + 24),
             static_cast<int>(survival.y + 116), 16, RAYWHITE);
    DrawText(T("Build your arsenal and defeat", "Monte seu arsenal e derrote"), static_cast<int>(survival.x + 24),
             static_cast<int>(survival.y + 146), 16, RAYWHITE);
    DrawText(T("milestone bosses before the final fight.", "os chefes antes da batalha final."), static_cast<int>(survival.x + 24),
             static_cast<int>(survival.y + 176), 16, RAYWHITE);
    if (DrawButton({survival.x + 28, survival.y + survival.height - 76,
                    survival.width - 56, 52}, T("PLAY SURVIVAL", "JOGAR SURVIVAL"), 21) || IsKeyPressed(KEY_ENTER))
        return MenuAction::PlaySurvival;

    DrawRectangleRounded(expedition, 0.08f, 10, Color{25, 22, 39, 245});
    DrawRectangleRoundedLinesEx(expedition, 0.08f, 10, 2.5f, Color{180, 105, 245, 220});
    DrawText("EXPEDITION", static_cast<int>(expedition.x + 24), static_cast<int>(expedition.y + 24),
             30, Color{210, 165, 255, 255});
    DrawText(T("AVAILABLE", "DISPONÍVEL"), static_cast<int>(expedition.x + 24),
             static_cast<int>(expedition.y + 66), 16, GREEN);
    DrawText(T("Choose a route through authored arenas.", "Escolha uma rota por arenas autorais."), static_cast<int>(expedition.x + 24),
             static_cast<int>(expedition.y + 116), 16, RAYWHITE);
    DrawText(T("Aim with mouse; LMB/RMB use two weapons.", "Mire com mouse; LMB/RMB usam duas armas."), static_cast<int>(expedition.x + 24),
             static_cast<int>(expedition.y + 146), 16, RAYWHITE);
    DrawText(T("Swap slots with Q and defeat the boss.", "Troque slots com Q e derrote o chefe."), static_cast<int>(expedition.x + 24),
             static_cast<int>(expedition.y + 176), 16, RAYWHITE);
    if (DrawButton({expedition.x + 28, expedition.y + expedition.height - 76,
                    expedition.width - 56, 52}, T("PLAY EXPEDITION", "JOGAR EXPEDIÇÃO"), 21))
        return MenuAction::PlayExpedition;

    if (DrawButton({GetScreenWidth() * 0.5f - 95.0f, GetScreenHeight() - 62.0f,
                    190.0f, 42.0f}, T("BACK", "VOLTAR"), 19)) return MenuAction::Back;
    return MenuAction::None;
}

int GameUI::DrawCharacterSelection() {
    DrawMenuBackdrop();
    DrawCentered(T("SELECT SURVIVOR", "SELECIONE O SOBREVIVENTE"), 30, 42, Color{218, 245, 255, 255});
    DrawCentered(T("Each survivor shares the same item pool and Phase Dash",
                   "Todos compartilham os mesmos itens e o Dash de Fase"), 78, 17,
                 Color{140, 185, 200, 255});
    const float gap = 18.0f;
    const float width = std::min(500.0f, (GetScreenWidth() - 70.0f - gap) * 0.5f);
    const float height = std::min(245.0f, (GetScreenHeight() - 170.0f - gap) * 0.5f);
    const float startX = (GetScreenWidth() - width * 2.0f - gap) * 0.5f;
    const float startY = 112.0f;
    for (int raw = 0; raw < static_cast<int>(CharacterId::Count); ++raw) {
        const CharacterDefinition& character = GetCharacterDefinition(static_cast<CharacterId>(raw));
        const int column = raw % 2;
        const int row = raw / 2;
        const Rectangle card{startX + column * (width + gap), startY + row * (height + gap), width, height};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), card);
        const bool compact = height < 215.0f;
        DrawRectangleRounded(card, 0.08f, 8, hovered ? Color{36, 48, 68, 250} : Color{18, 25, 40, 245});
        DrawRectangleRoundedLinesEx(card, 0.08f, 8, hovered ? 3.0f : 2.0f, character.color);
        const Vector2 portrait{card.x + (compact ? 48.0f : 66.0f), card.y + (compact ? 76.0f : 68.0f)};
        const float portraitRadius = compact ? 31.0f : 44.0f;
        DrawCircleV(portrait, portraitRadius, Color{character.color.r, character.color.g, character.color.b, 38});
        DrawPoly(portrait, 4 + raw, compact ? 23.0f : 31.0f, 45.0f + raw * 15.0f, character.color);
        DrawPolyLinesEx(portrait, 3 + raw, compact ? 14.0f : 19.0f, raw * 30.0f, 3.0f, WHITE);
        DrawText(TextFormat("%d", raw + 1), static_cast<int>(card.x + 12), static_cast<int>(card.y + 10), 18, GOLD);
        const float textX = card.x + (compact ? 96.0f : 125.0f);
        DrawText(K(character.name), static_cast<int>(textX), static_cast<int>(card.y + (compact ? 10.0f : 20.0f)),
                 compact ? 21 : 26, RAYWHITE);
        DrawText(TextFormat(T("Starts: %s", "Começa com: %s"), K(GetWeaponDefinition(character.startingWeapon).name)),
                 static_cast<int>(textX), static_cast<int>(card.y + (compact ? 39.0f : 56.0f)),
                 compact ? 14 : 16, character.color);
        DrawText(K(character.traitName), static_cast<int>(textX),
                 static_cast<int>(card.y + (compact ? 63.0f : 88.0f)), compact ? 15 : 17, GOLD);
        DrawWrappedText(K(character.traitDescription),
                        {textX, card.y + (compact ? 84.0f : 112.0f), card.x + card.width - textX - 12.0f,
                         compact ? 35.0f : 48.0f}, compact ? 12 : 13, LIGHTGRAY);
        DrawText(TextFormat(T("HP %+.0f%%  SPEED %+.0f%%  AREA %+.0f%%  DUR %+.0f%%",
                              "HP %+.0f%%  VEL %+.0f%%  AREA %+.0f%%  DUR %+.0f%%"), (character.maxHPMultiplier - 1.0f) * 100.0f,
                            (character.moveSpeedMultiplier - 1.0f) * 100.0f,
                            (character.areaMultiplier - 1.0f) * 100.0f,
                            (character.durationMultiplier - 1.0f) * 100.0f),
                 static_cast<int>(card.x + (compact ? 14.0f : 20.0f)),
                 static_cast<int>(card.y + height - (compact ? 42.0f : 52.0f)), compact ? 12 : 14, RAYWHITE);
        DrawText(TextFormat(T("ARMOR %+.0f  LUCK %+.2f  CRIT %+.0f%%", "ARMADURA %+.0f  SORTE %+.2f  CRIT %+.0f%%"), character.armorBonus,
                            character.luckBonus, character.criticalBonus * 100.0f),
                 static_cast<int>(card.x + (compact ? 14.0f : 20.0f)),
                 static_cast<int>(card.y + height - (compact ? 21.0f : 29.0f)), compact ? 12 : 14, RAYWHITE);
        if ((hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE + raw)) return raw;
    }
    if (DrawButton({GetScreenWidth() * 0.5f - 90.0f, GetScreenHeight() - 45.0f, 180.0f, 34.0f}, T("BACK", "VOLTAR"), 16)) return -2;
    return -1;
}

GameUI::SetupAction GameUI::DrawSurvivalSetup(CharacterId characterId,
                                               SurvivalRunConfig& editableConfig) {
    // The animated menu emblem occupies the same visual band as these selectors,
    // so this denser screen intentionally uses only the starfield backdrop.
    DrawMenuBackdrop(false);
    editableConfig.Sanitize();
    const CharacterDefinition& character = GetCharacterDefinition(characterId);
    const bool challengeLocked = editableConfig.challenge != ChallengeId::None;
    const SurvivalRunConfig resolved = ResolveSurvivalConfig(editableConfig);
    const RunModifiers preview = BuildRunModifiers(resolved);
    const float verticalOffset = std::clamp((GetScreenHeight() - 540.0f) * 0.33f, 0.0f, 70.0f);
    DrawCentered(T("SURVIVAL SETUP", "CONFIGURAÇÃO SURVIVAL"), static_cast<int>(24 + verticalOffset), 34, Color{218, 245, 255, 255});
    DrawCentered(TextFormat("%s  |  %s", K(character.name),
                            K(GetWeaponDefinition(character.startingWeapon).name)),
                 static_cast<int>(64 + verticalOffset), 17, character.color);
    const float center = GetScreenWidth() * 0.5f;
    const float labelX = center - 330.0f;
    auto selector = [&](float y, const char* label, const char* value) {
        DrawText(label, static_cast<int>(labelX), static_cast<int>(y + 8), 17, GRAY);
        DrawRectangleRounded({center - 130.0f, y, 260.0f, 36.0f}, 0.18f, 6, Color{24, 32, 49, 245});
        DrawText(value, static_cast<int>(center - MeasureText(value, 17) * 0.5f),
                 static_cast<int>(y + 9), 17, RAYWHITE);
        return std::array<Rectangle, 2>{{{center - 174.0f, y, 36.0f, 36.0f},
                                         {center + 138.0f, y, 36.0f, 36.0f}}};
    };
    auto clicked = [](Rectangle bounds, const char* symbol) {
        const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
        DrawRectangleRounded(bounds, 0.22f, 6, hover ? Color{105, 78, 155, 255} : Color{64, 53, 91, 255});
        const int fontSize = 22;
        DrawText(symbol, static_cast<int>(bounds.x + (bounds.width - MeasureText(symbol, fontSize)) * 0.5f),
                 static_cast<int>(bounds.y + 6.0f), fontSize, RAYWHITE);
        return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    };

    const auto difficultyButtons = selector(98.0f + verticalOffset, T("DIFFICULTY", "DIFICULDADE"),
        TextFormat("%s%s", K(DifficultyName(resolved.difficulty)),
                   challengeLocked ? T("  [LOCKED]", "  [BLOQUEADO]") : ""));
    if (!challengeLocked) {
        if (clicked(difficultyButtons[0], "<")) editableConfig.difficulty = static_cast<DifficultyTier>((static_cast<int>(editableConfig.difficulty) + 2) % 3);
        if (clicked(difficultyButtons[1], ">")) editableConfig.difficulty = static_cast<DifficultyTier>((static_cast<int>(editableConfig.difficulty) + 1) % 3);
    }
    const auto ascensionButtons = selector(144.0f + verticalOffset, T("ASCENSION", "ASCENSÃO"),
        TextFormat("A%d%s", resolved.ascension,
                   challengeLocked ? T("  [LOCKED]", "  [BLOQUEADO]") : ""));
    if (!challengeLocked) {
        if (clicked(ascensionButtons[0], "<")) editableConfig.ascension = std::max(0, editableConfig.ascension - 1);
        if (clicked(ascensionButtons[1], ">")) editableConfig.ascension = std::min(10, editableConfig.ascension + 1);
    }
    const auto challengeButtons = selector(190.0f + verticalOffset, T("RULESET", "CONJUNTO DE REGRAS"), K(ChallengeName(editableConfig.challenge)));
    if (clicked(challengeButtons[0], "<")) editableConfig.challenge = static_cast<ChallengeId>((static_cast<int>(editableConfig.challenge) + 6) % 7);
    if (clicked(challengeButtons[1], ">")) editableConfig.challenge = static_cast<ChallengeId>((static_cast<int>(editableConfig.challenge) + 1) % 7);
    if (editableConfig.challenge != ChallengeId::None) {
        const ChallengeDefinition& challenge = GetChallengeDefinition(editableConfig.challenge);
        DrawCentered(K(challenge.description), static_cast<int>(232 + verticalOffset), 14, Color{215, 185, 255, 255});
    } else {
        const Rectangle endless{center - 95.0f, 226.0f + verticalOffset, 190.0f, 34.0f};
        if (DrawButton(endless, editableConfig.endless ? T("ENDLESS: ON", "ENDLESS: LIGADO")
                                                       : T("ENDLESS: OFF", "ENDLESS: DESLIGADO"), 15))
            editableConfig.endless = !editableConfig.endless;
    }

    DrawText(TextFormat(T("MUTATORS %d/3%s", "MODIFICADORES %d/3%s"), resolved.MutatorCount(),
                        challengeLocked ? T("  [RULESET LOCKED]", "  [REGRAS BLOQUEADAS]") : ""),
             static_cast<int>(labelX), static_cast<int>(272 + verticalOffset), 16, challengeLocked ? VIOLET : GOLD);
    const float mutatorWidth = std::min(220.0f, (GetScreenWidth() - 90.0f) * 0.25f - 8.0f);
    const float mutatorStart = (GetScreenWidth() - (mutatorWidth * 4.0f + 24.0f)) * 0.5f;
    const char* mutatorHint = T("Hover a modifier to see its effect",
                                "Passe o mouse sobre um modificador para ver o efeito");
    for (int raw = 0; raw < static_cast<int>(MutatorId::Count); ++raw) {
        const MutatorId id = static_cast<MutatorId>(raw);
        const MutatorDefinition& definition = GetMutatorDefinition(id);
        const int column = raw % 4;
        const int row = raw / 4;
        const Rectangle card{mutatorStart + column * (mutatorWidth + 8.0f), 298.0f + verticalOffset + row * 58.0f,
                             mutatorWidth, 50.0f};
        const bool active = resolved.HasMutator(id);
        const bool incompatible = (id == MutatorId::HyperHorde && resolved.HasMutator(MutatorId::Titanic)) ||
                                  (id == MutatorId::Titanic && resolved.HasMutator(MutatorId::HyperHorde));
        const bool unavailable = !active && (incompatible || resolved.MutatorCount() >= 3);
        const bool pointed = CheckCollisionPointRec(GetMousePosition(), card);
        const bool hover = !challengeLocked && !unavailable && pointed;
        DrawRectangleRounded(card, 0.12f, 5, active ? Color{67, 45, 91, 250}
                                                     : (unavailable ? Color{15, 18, 27, 220}
                                                                    : Color{20, 27, 42, 245}));
        DrawRectangleRoundedLinesEx(card, 0.12f, 5, active ? 2.5f : 1.0f,
                                    active ? VIOLET : (hover ? SKYBLUE :
                                                       (unavailable ? Color{45, 48, 57, 255}
                                                                    : Color{70, 80, 100, 255})));
        DrawText(K(definition.name), static_cast<int>(card.x + 7), static_cast<int>(card.y + 7), 13,
                 active ? RAYWHITE : LIGHTGRAY);
        DrawText(TextFormat(T("SCORE +%d", "PONTOS +%d"), definition.scoreImpact), static_cast<int>(card.x + 7),
                 static_cast<int>(card.y + 29), 11, active ? GOLD : GRAY);
        if (pointed) {
            mutatorHint = incompatible ? T("Incompatible with the selected modifier",
                                           "Incompatível com o modificador selecionado")
                                       : (unavailable ? T("Maximum of three modifiers reached",
                                                          "Limite de três modificadores atingido")
                                                      : K(definition.description));
        }
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) editableConfig.ToggleMutator(id);
    }
    DrawCentered(mutatorHint, static_cast<int>(399 + verticalOffset), 12, LIGHTGRAY);
    DrawCentered(TextFormat(T("Score multiplier preview  x%.2f", "Prévia do multiplicador de pontos  x%.2f"), preview.scoreMultiplier), static_cast<int>(420 + verticalOffset), 15, GREEN);
    if (DrawButton({center - 310.0f, 452.0f + verticalOffset, 190.0f, 48.0f}, T("BACK", "VOLTAR"), 18)) return SetupAction::Back;
    if (DrawButton({center - 95.0f, 452.0f + verticalOffset, 190.0f, 48.0f}, T("QUICK START", "INÍCIO RÁPIDO"), 18)) return SetupAction::QuickStart;
    if (DrawButton({center + 120.0f, 452.0f + verticalOffset, 190.0f, 48.0f}, T("START RUN", "INICIAR PARTIDA"), 18)) return SetupAction::Start;
    return SetupAction::None;
}

GameUI::MenuAction GameUI::DrawPause() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{5, 8, 14, 220});
    DrawCentered(T("PAUSED", "PAUSADO"), 88, 54, Color{155, 245, 225, 255});
    const float x = GetScreenWidth() * 0.5f - 135.0f;
    if (DrawButton({x, 190, 270, 52}, T("RESUME", "CONTINUAR"), 23)) return MenuAction::Resume;
    if (DrawButton({x, 255, 270, 52}, T("SETTINGS", "CONFIGURAÇÕES"), 23)) return MenuAction::Settings;
    if (DrawButton({x, 320, 270, 52}, T("RESTART", "REINICIAR"), 23)) return MenuAction::Restart;
    if (DrawButton({x, 385, 270, 52}, T("MAIN MENU", "MENU PRINCIPAL"), 23)) return MenuAction::MainMenu;
    if (DrawButton({x, 450, 270, 52}, T("QUIT", "SAIR"), 23)) return MenuAction::Quit;
    return MenuAction::None;
}

GameUI::MenuAction GameUI::DrawSettings(GameSettings& settings) {
    DrawMenuBackdrop(false);
    DrawCentered(T("SETTINGS", "CONFIGURAÇÕES"), 34, 42, Color{155, 245, 225, 255});
    const float x = GetScreenWidth() * 0.5f - 190.0f;
    DrawSlider(T("MASTER VOLUME", "VOLUME GERAL"), {x, 105, 330, 16}, settings.masterVolume);
    DrawSlider(T("MUSIC", "MÚSICA"), {x, 163, 330, 16}, settings.musicVolume);
    DrawSlider(T("SFX", "EFEITOS SONOROS"), {x, 221, 330, 16}, settings.sfxVolume);
    DrawSlider(T("SCREEN SHAKE", "TREMOR DE TELA"), {x, 279, 330, 16}, settings.screenShake);
    const char* fullscreen = settings.fullscreen ? T("FULLSCREEN: ON", "TELA CHEIA: LIGADA")
                                                  : T("FULLSCREEN: OFF", "TELA CHEIA: DESLIGADA");
    if (DrawButton({x, 323, 380, 44}, fullscreen, 18)) settings.fullscreen = !settings.fullscreen;
    const char* language = settings.language == GameLanguage::PortugueseBrazil
                               ? "IDIOMA: PORTUGUÊS (BRASIL)" : "LANGUAGE: ENGLISH";
    if (DrawButton({x, 377, 380, 44}, language, 17)) {
        settings.language = settings.language == GameLanguage::English
                                ? GameLanguage::PortugueseBrazil : GameLanguage::English;
        Localization::SetLanguage(settings.language);
    }
    const float backY = std::min(445.0f, GetScreenHeight() - 64.0f);
    if (DrawButton({x + 55, backY, 270, 50}, T("BACK", "VOLTAR"), 22)) return MenuAction::Back;
    if (GetScreenHeight() >= 620)
        DrawCentered(T("Settings are saved beside the executable",
                       "As configurações são salvas ao lado do executável"), GetScreenHeight() - 35, 15, GRAY);
    return MenuAction::None;
}

void GameUI::DrawHUD(const GameSession& session, const ModeHUDData& mode) {
    const PlayerStats& stats = session.GetPlayer().stats;
    DrawText(TextFormat("HP %.0f / %.0f", stats.currentHP, stats.maxHP), 24, 20, 20, RAYWHITE);
    DrawBar({24.0f, 47.0f, 290.0f, 22.0f}, stats.currentHP / stats.maxHP,
            Color{210, 65, 78, 255});

    DrawText(TextFormat("LV %d", stats.level), 24, 82, 22, Color{245, 225, 130, 255});
    DrawText(TextFormat("XP %d / %d", stats.currentXP, stats.xpRequired), 102, 85, 17,
             Color{205, 230, 220, 255});
    DrawBar({24.0f, 112.0f, 290.0f, 17.0f},
            static_cast<float>(stats.currentXP) / static_cast<float>(stats.xpRequired),
            Color{58, 220, 170, 255});

    if (mode.mode == GameModeType::Survival) {
        const int totalSeconds = static_cast<int>(session.ElapsedTime());
        const char* timeText = TextFormat("%02d:%02d", totalSeconds / 60, totalSeconds % 60);
        DrawText(timeText, (GetScreenWidth() - MeasureText(timeText, 32)) / 2, 18, 32, RAYWHITE);
    } else {
        DrawCentered(TextFormat(T("STAGE %d / %d", "FASE %d / %d"), mode.progressIndex, mode.stageCount), 16, 28, RAYWHITE);
        DrawCentered(K(mode.progressName), 48, 17, Color{195, 145, 255, 255});
        const char* objective = mode.exitOpen
            ? T("EXIT OPEN", "SAÍDA ABERTA")
            : TextFormat(T("%s  |  Enemies %d", "%s  |  Inimigos %d"),
                         K(mode.objective), mode.enemiesRemaining);
        DrawCentered(objective, 72, 15, mode.exitOpen ? GREEN : LIGHTGRAY);
    }
    const char* killsText = TextFormat(T("Kills: %d", "Abates: %d"), session.Kills());
    DrawText(killsText, GetScreenWidth() - MeasureText(killsText, 22) - 24, 24, 22, RAYWHITE);
    const char* modeText = TextFormat("%s  |  %s %d", GameModeName(mode.mode),
                                      K(mode.progressLabel), mode.progressIndex);
    DrawText(modeText, GetScreenWidth() - MeasureText(modeText, 15) - 24, 54, 15,
             Color{120, 190, 190, 220});
    DrawText(TextFormat("%s | %s", K(CharacterName(session.SelectedCharacter())),
                        K(GetCharacterDefinition(session.SelectedCharacter()).traitName)),
             GetScreenWidth() - MeasureText(TextFormat("%s | %s", K(CharacterName(session.SelectedCharacter())),
                         K(GetCharacterDefinition(session.SelectedCharacter()).traitName)), 14) - 24,
             76, 14, GetCharacterDefinition(session.SelectedCharacter()).color);
    if (mode.mode == GameModeType::Survival) DrawText(TextFormat(T("%s A%d%s  |  SCORE %lld", "%s A%d%s  |  PONTOS %lld"), K(DifficultyName(mode.difficulty)), mode.ascension,
                        mode.endless ? TextFormat(" ENDLESS C%d", mode.endlessCycle) : "",
                        session.CurrentScore().finalScore),
             GetScreenWidth() - MeasureText(TextFormat(T("%s A%d%s  |  SCORE %lld", "%s A%d%s  |  PONTOS %lld"), K(DifficultyName(mode.difficulty)),
                         mode.ascension, mode.endless ? TextFormat(" ENDLESS C%d", mode.endlessCycle) : "",
                         session.CurrentScore().finalScore), 13) - 24,
             98, 13, GOLD);

    const float dashRatio = 1.0f - session.GetPlayer().DashCooldownRemaining() /
                                      session.GetPlayer().DashCooldownDuration();
    const Rectangle dashBox{24.0f, 143.0f, 150.0f, 32.0f};
    DrawRectangleRounded(dashBox, 0.22f, 6, Color{13, 18, 30, 220});
    DrawRectangleRec({dashBox.x, dashBox.y + 27.0f, dashBox.width * std::clamp(dashRatio, 0.0f, 1.0f), 5.0f},
                     dashRatio >= 1.0f ? SKYBLUE : Color{75, 90, 125, 255});
    DrawText(session.GetPlayer().DashCooldownRemaining() <= 0.0f
                 ? T("SPACE  PHASE DASH", "ESPAÇO  DASH DE FASE")
                 : TextFormat("DASH  %.1fs", session.GetPlayer().DashCooldownRemaining()),
             34, 151, 14, RAYWHITE);

    if (mode.eventActive) {
        DrawCentered(K(mode.eventName), 122, 30,
                     std::strcmp(mode.eventName, "BLOOD MOON") == 0 ? Color{255, 80, 95, 255} : GOLD);
        DrawCentered(TextFormat("%s  %.0fs", K(mode.eventSubtitle), std::max(0.0f, mode.eventRemaining)),
                     153, 16, RAYWHITE);
    } else if (std::strcmp(mode.specialWaveName, "NONE") != 0) {
        DrawCentered(TextFormat("%s  %.0fs", K(mode.specialWaveName),
                                std::max(0.0f, mode.specialWaveRemaining)), 128, 27, ORANGE);
    }

    if (const Enemy* miniboss = session.Enemies().ActiveMiniboss()) {
        const float width = std::min(440.0f, GetScreenWidth() * 0.42f);
        const float x = (GetScreenWidth() - width) * 0.5f;
        DrawCentered(K(GetEnemyDefinition(miniboss->type).name), session.Bosses().IsActive() ? 116 : 68,
                     18, GOLD);
        DrawBar({x, session.Bosses().IsActive() ? 138.0f : 91.0f, width, 11.0f},
                miniboss->hp / miniboss->maxHP, Color{220, 156, 62, 255});
    }

    if (session.Bosses().IsActive()) {
        const Boss& boss = session.Bosses().ActiveBoss();
        const float width = std::min(620.0f, GetScreenWidth() * 0.55f);
        const float x = (GetScreenWidth() - width) * 0.5f;
        DrawCentered(K(boss.name), 60, 21, boss.phase == 2 ? GOLD : RAYWHITE);
        DrawBar({x, 88.0f, width, 18.0f}, boss.currentHP / boss.maxHP,
                boss.type == BossType::FlameWyrm ? ORANGE : VIOLET);
        if (session.Bosses().IntroRemaining() > 0.0f)
            DrawCentered(boss.phase == 2 ? T("PHASE II", "FASE II") : K(boss.name), 148, 44,
                         boss.type == BossType::FlameWyrm ? ORANGE : VIOLET);
    }

    const WeaponManager& loadout = session.Loadout();
    const int equipmentY = GetScreenHeight() - 76;
    const bool manual = mode.mode == GameModeType::Expedition;
    const float weaponStartX = manual ? GetScreenWidth() * 0.5f - 180.0f : 20.0f;
    DrawText(manual ? T("MANUAL WEAPONS", "ARMAS MANUAIS") : T("WEAPONS", "ARMAS"),
             static_cast<int>(weaponStartX), equipmentY - 22, 15, Color{180, 205, 245, 255});
    const int visibleWeaponSlots = manual ? 2 : WeaponManager::MaxWeaponSlots;
    for (int index = 0; index < visibleWeaponSlots; ++index) {
        const float width = manual ? 170.0f : 46.0f;
        const Rectangle slot{weaponStartX + index * (manual ? 190.0f : 54.0f),
                             static_cast<float>(equipmentY), width, 46.0f};
        DrawRectangleRounded(slot, 0.18f, 6, Color{10, 15, 24, 225});
        DrawRectangleRoundedLinesEx(slot, 0.18f, 6, 1.5f, Color{80, 110, 145, 210});
        const char* binding = index == 0 ? "LMB" : "RMB";
        if (manual) DrawText(binding, static_cast<int>(slot.x + 8), static_cast<int>(slot.y + 5), 12, SKYBLUE);
        if (index >= loadout.WeaponCount()) {
            if (manual) DrawText(T("EMPTY", "VAZIO"), static_cast<int>(slot.x + 55),
                                 static_cast<int>(slot.y + 15), 15, GRAY);
            continue;
        }
        const WeaponInstance& weapon = loadout.Weapons()[static_cast<std::size_t>(index)];
        const bool evolved = weapon.evolved != EvolvedWeaponType::None;
        VisualStyle::DrawWeaponIcon(weapon.type, weapon.evolved,
                                    {slot.x + 5, slot.y + 4, 36, 32}, static_cast<float>(GetTime()));
        if (manual) {
            DrawText(K(evolved ? EvolvedWeaponName(weapon.evolved) : GetWeaponDefinition(weapon.type).name),
                     static_cast<int>(slot.x + 48), static_cast<int>(slot.y + 7), 13, RAYWHITE);
            const float cooldown = loadout.SlotCooldown(index);
            DrawText(cooldown <= 0.0f ? T("READY", "PRONTA") : TextFormat("%.1fs", cooldown),
                     static_cast<int>(slot.x + 48), static_cast<int>(slot.y + 26), 13,
                     cooldown <= 0.0f ? GREEN : ORANGE);
        } else DrawText(evolved ? "MAX" : TextFormat("%d", weapon.level), static_cast<int>(slot.x + 3),
                          static_cast<int>(slot.y + 30), 12, evolved ? GOLD : RAYWHITE);
    }
    const int passiveX = GetScreenWidth() - 20 - WeaponManager::MaxPassiveSlots * 54;
    DrawText(T("PASSIVES", "PASSIVAS"), passiveX, equipmentY - 22, 15, Color{205, 180, 245, 255});
    for (int index = 0; index < WeaponManager::MaxPassiveSlots; ++index) {
        const Rectangle slot{static_cast<float>(passiveX + index * 54), static_cast<float>(equipmentY), 46, 46};
        DrawRectangleRounded(slot, 0.18f, 6, Color{10, 15, 24, 225});
        DrawRectangleRoundedLinesEx(slot, 0.18f, 6, 1.5f, Color{115, 85, 145, 210});
        if (index >= loadout.PassiveCount()) continue;
        const PassiveInstance& passive = loadout.Passives()[static_cast<std::size_t>(index)];
        VisualStyle::DrawPassiveIcon(passive.type, {slot.x + 5, slot.y + 4, 36, 32}, static_cast<float>(GetTime()));
        DrawText(TextFormat("%d", passive.level), static_cast<int>(slot.x + 3),
                 static_cast<int>(slot.y + 30), 12, RAYWHITE);
    }
}

int GameUI::DrawLevelUp(const GameSession& session) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{9, 7, 15, 220});
    DrawCentered(T("LEVEL UP!", "SUBIU DE NIVEL!"), 90, 48, Color{246, 218, 113, 255});
    DrawCentered(T("Choose an upgrade", "Escolha uma melhoria"), 150, 22, RAYWHITE);

    const auto& choices = session.UpgradeChoices();
    constexpr float cardWidth = 310.0f;
    constexpr float cardHeight = 220.0f;
    constexpr float gap = 24.0f;
    const float startX = (GetScreenWidth() - (cardWidth * 3.0f + gap * 2.0f)) * 0.5f;
    for (int index = 0; index < session.UpgradeChoiceCount(); ++index) {
        const Rectangle card{startX + index * (cardWidth + gap), 235.0f, cardWidth, cardHeight};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), card);
        DrawRectangleRounded(card, 0.12f, 8, hovered ? Color{80, 65, 112, 255}
                                                       : Color{47, 42, 67, 255});
        DrawRectangleRoundedLinesEx(card, 0.12f, 8, 3.0f,
                                    hovered ? Color{230, 204, 255, 255}
                                            : Color{115, 99, 145, 255});
        DrawText(TextFormat("%d", index + 1), static_cast<int>(card.x + 18.0f),
                 static_cast<int>(card.y + 15.0f), 24, Color{246, 218, 113, 255});
        const UpgradeChoice& choice = choices[static_cast<std::size_t>(index)];
        if (choice.kind == UpgradeKind::Weapon)
            VisualStyle::DrawWeaponIcon(choice.weapon, EvolvedWeaponType::None,
                                        {card.x + 126, card.y + 12, 58, 40}, static_cast<float>(GetTime()));
        else VisualStyle::DrawPassiveIcon(choice.passive,
                                          {card.x + 126, card.y + 12, 58, 40}, static_cast<float>(GetTime()));
        const Color rarityColor = RarityColor(choice.rarity);
        DrawCircleV({card.x + card.width - 28.0f, card.y + 28.0f}, 11.0f, rarityColor);
        DrawText(K(choice.name.c_str()), static_cast<int>(card.x + 20.0f),
                 static_cast<int>(card.y + 58.0f), 22, RAYWHITE);
        DrawText(TextFormat("%s  |  %s", K(RarityName(choice.rarity)), K(choice.typeLabel.c_str())),
                 static_cast<int>(card.x + 20.0f), static_cast<int>(card.y + 93.0f), 16,
                 rarityColor);
        if (choice.currentLevel > 0) {
            DrawText(TextFormat("Lv %d -> %d", choice.currentLevel, choice.newLevel),
                     static_cast<int>(card.x + 20.0f), static_cast<int>(card.y + 122.0f),
                     18, Color{215, 215, 230, 255});
        } else {
            DrawText("Lv 1", static_cast<int>(card.x + 20.0f),
                     static_cast<int>(card.y + 122.0f), 18, Color{215, 215, 230, 255});
        }
        DrawText(K(choice.description.c_str()), static_cast<int>(card.x + 20.0f),
                 static_cast<int>(card.y + 150.0f), 13, Color{185, 185, 200, 255});
        const std::string localizedEffect = LocalizedEffect(choice.effect);
        DrawText(localizedEffect.c_str(), static_cast<int>(card.x + 20.0f),
                 static_cast<int>(card.y + 178.0f), 15, Color{170, 230, 210, 255});
        if ((hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ||
            IsKeyPressed(KEY_ONE + index)) {
            return index;
        }
    }
    return -1;
}

int GameUI::DrawChestReward(const GameSession& session, GameModeType mode) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{10, 7, 18, 235});
    const int titleSize = 48 + static_cast<int>((std::sin(static_cast<float>(GetTime()) * 5.0f) + 1.0f) * 2.0f);
    DrawCentered(mode == GameModeType::Expedition ? T("STAGE REWARD", "RECOMPENSA DA FASE")
                                                   : T("TREASURE CHEST", "BAÚ DO TESOURO"), 72, titleSize, GOLD);
    DrawCentered(T("Choose your reward", "Escolha sua recompensa"), 132, 22, RAYWHITE);
    constexpr float cardWidth = 310.0f;
    constexpr float cardHeight = 205.0f;
    constexpr float gap = 24.0f;
    const int count = session.ChestChoiceCount();
    const float totalWidth = cardWidth * count + gap * std::max(0, count - 1);
    const float startX = (GetScreenWidth() - totalWidth) * 0.5f;
    for (int index = 0; index < count; ++index) {
        const Rectangle card{startX + index * (cardWidth + gap), 230.0f, cardWidth, cardHeight};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), card);
        DrawRectangleRounded(card, 0.12f, 8, hovered ? Color{92, 68, 105, 255}
                                                       : Color{52, 41, 67, 255});
        const ChestRewardChoice& choice = session.ChestChoices()[static_cast<std::size_t>(index)];
        DrawRectangleRoundedLinesEx(card, 0.12f, 8, 3.0f, hovered ? YELLOW : RarityColor(choice.rarity));
        DrawText(TextFormat("%d", index + 1), static_cast<int>(card.x + 18),
                 static_cast<int>(card.y + 16), 24, GOLD);
        DrawText(choice.kind == ChestRewardKind::Evolution ? T("EVOLUTION", "EVOLUÇÃO")
                                                           : T("REWARD", "RECOMPENSA"),
                 static_cast<int>(card.x + 20), static_cast<int>(card.y + 53), 17,
                 choice.kind == ChestRewardKind::Evolution ? VIOLET : SKYBLUE);
        DrawText(K(RarityName(choice.rarity)), static_cast<int>(card.x + card.width - 85),
                 static_cast<int>(card.y + 56), 13, RarityColor(choice.rarity));
        DrawText(K(choice.name.c_str()), static_cast<int>(card.x + 20),
                 static_cast<int>(card.y + 88), 22, RAYWHITE);
        const std::string localizedDescription = LocalizedEffect(choice.description);
        DrawText(localizedDescription.c_str(), static_cast<int>(card.x + 20),
                 static_cast<int>(card.y + 132), 14, Color{195, 205, 220, 255});
        if ((hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE + index))
            return index;
    }
    return -1;
}

int GameUI::DrawWeaponReplacement(const GameSession& session) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{8, 6, 15, 242});
    DrawCentered(T("WEAPON SLOTS FULL", "SLOTS DE ARMA CHEIOS"), 70, 42, GOLD);
    const WeaponType incoming = session.PendingReplacementWeapon();
    DrawCentered(TextFormat(T("Equip %s by replacing one weapon", "Equipe %s substituindo uma arma"),
                            K(GetWeaponDefinition(incoming).name)), 128, 21, RAYWHITE);
    const float center = GetScreenWidth() * 0.5f;
    for (int slotIndex = 0; slotIndex < 2; ++slotIndex) {
        const Rectangle card{center - 330.0f + slotIndex * 350.0f, 205.0f, 310.0f, 190.0f};
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), card);
        DrawRectangleRounded(card, 0.12f, 8, hovered ? Color{76, 55, 98, 255} : Color{37, 31, 53, 255});
        DrawRectangleRoundedLinesEx(card, 0.12f, 8, 3.0f, hovered ? GOLD : VIOLET);
        const WeaponInstance& current = session.Loadout().Weapons()[static_cast<std::size_t>(slotIndex)];
        DrawText(slotIndex == 0 ? "LMB" : "RMB", static_cast<int>(card.x + 18),
                 static_cast<int>(card.y + 16), 18, SKYBLUE);
        VisualStyle::DrawWeaponIcon(current.type, current.evolved,
                                    {card.x + 119, card.y + 26, 72, 62}, static_cast<float>(GetTime()));
        DrawCentered(K(current.evolved != EvolvedWeaponType::None
                           ? EvolvedWeaponName(current.evolved) : GetWeaponDefinition(current.type).name),
                     static_cast<int>(card.y + 105), 22, RAYWHITE);
        if (current.evolved != EvolvedWeaponType::None)
            DrawText(T("EVOLVED — evolution will be lost", "EVOLUÍDA — a evolução será perdida"),
                     static_cast<int>(card.x + 22), static_cast<int>(card.y + 132), 13,
                     Color{255, 120, 135, 255});
        const char* confirm = T("CONFIRM REPLACEMENT", "CONFIRMAR SUBSTITUIÇÃO");
        DrawText(confirm, static_cast<int>(card.x + (card.width - MeasureText(confirm, 14)) * 0.5f),
                 static_cast<int>(card.y + 150), 14, hovered ? GOLD : LIGHTGRAY);
        if ((hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE + slotIndex))
            return slotIndex;
    }
    if (DrawButton({center - 110.0f, 440.0f, 220.0f, 48.0f}, T("CANCEL", "CANCELAR"), 19))
        return -2;
    return -1;
}

int GameUI::DrawExpeditionRoute(const ExpeditionDirector& route) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{5, 7, 16, 248});
    DrawCentered(T("CHOOSE YOUR ROUTE", "ESCOLHA SUA ROTA"), 54, 44, Color{205, 165, 255, 255});
    DrawCentered(TextFormat(T("Expedition depth %d / %d", "Profundidade da expedição %d / %d"),
                            route.StageIndex() + 1, route.StageCount()), 108, 18, LIGHTGRAY);
    const auto& nodes = route.RouteNodes();
    const float left = std::max(75.0f, GetScreenWidth() * 0.08f);
    const float right = GetScreenWidth() - left;
    const float top = 190.0f;
    const float bottom = GetScreenHeight() - 155.0f;
    auto position = [&](const ExpeditionRouteNode& node) {
        int count = 0;
        for (const auto& candidate : nodes) if (candidate.layer == node.layer) ++count;
        const float x = left + (right - left) * (static_cast<float>(node.layer) / 6.0f);
        const float y = count == 1 ? (top + bottom) * 0.5f
                                   : top + (bottom - top) * (static_cast<float>(node.column) / (count - 1));
        return Vector2{x, y};
    };
    for (const auto& node : nodes) {
        const Vector2 from = position(node);
        for (int i = 0; i < node.connectionCount; ++i) {
            const auto& target = nodes[static_cast<std::size_t>(node.connections[static_cast<std::size_t>(i)])];
            const bool path = node.state == ExpeditionRouteNodeState::Completed &&
                              (target.state == ExpeditionRouteNodeState::Completed ||
                               target.state == ExpeditionRouteNodeState::Current);
            DrawLineEx(from, position(target), path ? 4.0f : 2.0f,
                       path ? GOLD : Color{55, 65, 88, 210});
        }
    }
    int hoveredId = -1;
    for (const auto& node : nodes) {
        const Vector2 p = position(node);
        const bool hovered = CheckCollisionPointCircle(GetMousePosition(), p, 27.0f);
        if (hovered) hoveredId = node.id;
        Color typeColor = node.type == ExpeditionEncounterType::Elite ? GOLD :
                          node.type == ExpeditionEncounterType::Reward ? Color{92, 236, 169, 255} :
                          node.type == ExpeditionEncounterType::Boss ? Color{235, 75, 106, 255} : SKYBLUE;
        Color fill = Color{22, 28, 43, 255};
        if (node.state == ExpeditionRouteNodeState::Unavailable) typeColor = Color{70, 76, 92, 255};
        if (node.state == ExpeditionRouteNodeState::Completed) fill = Color{75, 61, 35, 255};
        if (node.state == ExpeditionRouteNodeState::Current) fill = Color{75, 43, 95, 255};
        if (node.state == ExpeditionRouteNodeState::Available) fill = Color{35, 64, 79, 255};
        DrawCircleV(p, hovered ? 27.0f : 23.0f, fill);
        DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), hovered ? 27.0f : 23.0f, typeColor);
        const char icon = node.type == ExpeditionEncounterType::Elite ? 'E' :
                          node.type == ExpeditionEncounterType::Reward ? '$' :
                          node.type == ExpeditionEncounterType::Boss ? 'B' : 'C';
        DrawText(TextFormat("%c", icon), static_cast<int>(p.x) - 7, static_cast<int>(p.y) - 11, 22, typeColor);
        if (IsKeyDown(KEY_F3)) DrawText(TextFormat("%d", node.id), static_cast<int>(p.x) - 6,
                                        static_cast<int>(p.y) + 31, 12, GRAY);
    }
    for (int option = 0; option < route.RouteOptionCount(); ++option) {
        const auto& node = route.RouteOptionNode(option);
        const Vector2 p = position(node);
        DrawText(TextFormat("%d", option + 1), static_cast<int>(p.x) - 4,
                 static_cast<int>(p.y) - 48, 16, WHITE);
        if ((CheckCollisionPointCircle(GetMousePosition(), p, 27.0f) &&
             IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE + option)) return option;
    }
    if (hoveredId >= 0) {
        const auto& node = nodes[static_cast<std::size_t>(hoveredId)];
        const char* type = node.type == ExpeditionEncounterType::Elite ? T("ELITE", "ELITE") :
                           node.type == ExpeditionEncounterType::Reward ? T("REWARD", "RECOMPENSA") :
                           node.type == ExpeditionEncounterType::Boss ? T("BOSS", "CHEFE") :
                           T("COMBAT", "COMBATE");
        const char* detail = node.type == ExpeditionEncounterType::Elite
            ? T("High risk • enhanced reward", "Alto risco • recompensa melhorada")
            : node.type == ExpeditionEncounterType::Reward
            ? T("No combat • premium reward", "Sem combate • recompensa premium")
            : node.type == ExpeditionEncounterType::Boss
            ? T("Final confrontation", "Confronto final")
            : T("Standard encounter • normal reward", "Encontro padrão • recompensa normal");
        DrawCentered(TextFormat("%s  —  %s", type, detail), GetScreenHeight() - 105, 18, RAYWHITE);
    }
    DrawCentered(T("Choose only a connected highlighted node", "Escolha apenas um nó conectado destacado"),
                 GetScreenHeight() - 77, 15, LIGHTGRAY);
    DrawCentered(TextFormat("SEED %u", route.Seed()), GetScreenHeight() - 46, 14, Color{110, 125, 150, 255});
    return -1;
}

GameUI::MenuAction GameUI::DrawGameOver(const GameSession& session, const ModeHUDData& mode) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{15, 8, 13, 235});
    DrawCentered(T("GAME OVER", "FIM DE JOGO"), 52, 54, Color{238, 83, 99, 255});
    const SurvivalRunConfig& config = session.SurvivalConfig();
    const ScoreBreakdown score = session.CurrentScore();
    DrawCentered(mode.mode == GameModeType::Expedition
        ? TextFormat(T("EXPEDITION  |  STAGE %d / %d", "EXPEDIÇÃO  |  FASE %d / %d"), mode.progressIndex, mode.stageCount)
        : TextFormat(T("MODE %s  |  %s A%d%s", "MODO %s  |  %s A%d%s"), GameModeName(mode.mode), K(DifficultyName(config.difficulty)),
                            config.ascension, config.endless ? " ENDLESS" : ""), 116, 17,
                 Color{145, 205, 195, 255});
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat(T("SURVIVOR %s  |  SEED %u", "SOBREVIVENTE %s  |  SEED %u"),
                                  K(CharacterName(session.SelectedCharacter())), mode.runSeed)
                     : TextFormat(T("SURVIVOR %s  |  RULESET %s", "SOBREVIVENTE %s  |  REGRAS %s"), K(CharacterName(session.SelectedCharacter())),
                                  K(ChallengeName(config.challenge))), 142, 16,
                 GetCharacterDefinition(session.SelectedCharacter()).color);

    const int seconds = static_cast<int>(session.ElapsedTime());
    DrawCentered(TextFormat(T("Time Survived  %02d:%02d", "Tempo sobrevivido  %02d:%02d"), seconds / 60, seconds % 60),
                 176, 23, RAYWHITE);
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat(T("Level %d   Kills %d   Stages cleared %d", "Nível %d   Abates %d   Fases concluídas %d"),
                                  session.GetPlayer().stats.level, session.Kills(), mode.stagesCompleted)
                     : TextFormat(T("Level %d   Kills %d   Bosses %d   Cycles %d", "Nivel %d   Abates %d   Chefes %d   Ciclos %d"), session.GetPlayer().stats.level,
                                  session.Kills(), session.Statistics().bossesKilled,
                                  session.EndlessCyclesCompleted()), 207, 20, RAYWHITE);
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat(T("Damage dealt %.0f   Taken %.0f", "Dano causado %.0f   Recebido %.0f"),
                                  session.Statistics().damageDealt, session.Statistics().damageTaken)
                     : TextFormat(T("BASE %lld  x%.2f DIFF  x%.2f ASC  x%.2f RULES", "BASE %lld  x%.2f DIF  x%.2f ASC  x%.2f REGRAS"), score.baseScore,
                                  score.difficultyMultiplier, score.ascensionMultiplier,
                                  score.mutatorMultiplier), 246, 17, LIGHTGRAY);
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? T("EXPEDITION FAILED", "EXPEDIÇÃO FRACASSADA")
                     : TextFormat(T("FINAL SCORE  %lld", "PONTUAÇÃO FINAL  %lld"), score.finalScore), 277, 30, GOLD);
    if (mode.mode == GameModeType::Survival)
        DrawCentered(TextFormat(T("MUTATORS  %s", "MODIFICADORES  %s"), MutatorSummary(config).c_str()), 319, 13, VIOLET);

    const float centerX = GetScreenWidth() * 0.5f;
    if (DrawButton({centerX - 145.0f, 378.0f, 290.0f, 56.0f}, T("RESTART", "REINICIAR")) ||
        IsKeyPressed(KEY_ENTER)) {
        return MenuAction::Restart;
    }
    if (DrawButton({centerX - 145.0f, 450.0f, 290.0f, 56.0f}, T("MAIN MENU", "MENU PRINCIPAL"))) {
        return MenuAction::MainMenu;
    }
    return MenuAction::None;
}

GameUI::MenuAction GameUI::DrawVictory(const GameSession& session, const ModeHUDData& mode) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{7, 17, 24, 238});
    DrawCentered(T("VICTORY", "VITÓRIA"), 32, 54, Color{102, 245, 190, 255});
    const SurvivalRunConfig& config = session.SurvivalConfig();
    const ScoreBreakdown score = session.CurrentScore();
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? T("The Void Herald has fallen", "O Arauto do Vazio foi derrotado")
                     : T("The Ascended Herald has fallen", "O Arauto Ascendido foi derrotado"), 92, 18, Color{175, 205, 215, 255});
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat(T("EXPEDITION COMPLETE  |  %d STAGES", "EXPEDIÇÃO CONCLUÍDA  |  %d FASES"), mode.stageCount)
                     : TextFormat("%s  |  %s A%d  |  %s", GameModeName(mode.mode), K(DifficultyName(config.difficulty)),
                            config.ascension, K(ChallengeName(config.challenge))), 119, 16,
                 Color{145, 205, 195, 255});
    DrawCentered(TextFormat(T("SURVIVOR  %s", "SOBREVIVENTE  %s"), K(CharacterName(session.SelectedCharacter()))), 143, 16,
                 GetCharacterDefinition(session.SelectedCharacter()).color);
    const RunStatistics& stats = session.Statistics();
    const int shift = std::max(0, static_cast<int>((720 - GetScreenHeight()) * 0.28f));
    DrawCentered(TextFormat(T("Time %02d:%02d", "Tempo %02d:%02d"), static_cast<int>(session.ElapsedTime()) / 60,
                            static_cast<int>(session.ElapsedTime()) % 60), 171 - shift, 21, RAYWHITE);
    DrawCentered(TextFormat(T("Level %d   Kills %d   Elites %d", "Nivel %d   Abates %d   Elites %d"), stats.levelReached,
                            stats.enemiesKilled, stats.elitesKilled), 210 - shift, 23, RAYWHITE);
    DrawCentered(TextFormat(T("Bosses Defeated %d   Evolutions %d   Chests %d", "Chefes derrotados %d   Evoluções %d   Baús %d"),
                            stats.bossesKilled, stats.evolutionsObtained, stats.chestsOpened),
                 245 - shift, 23, RAYWHITE);
    DrawCentered(TextFormat(T("Damage Dealt %.0f   Taken %.0f", "Dano causado %.0f   Recebido %.0f"), stats.damageDealt,
                            stats.damageTaken), 280 - shift, 23, RAYWHITE);
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat(T("%d NODES CLEARED", "%d NÓS CONCLUÍDOS"), mode.stagesCompleted)
                     : TextFormat(T("FINAL SCORE %lld", "PONTUAÇÃO FINAL %lld"), score.finalScore), 316 - shift, 27, GOLD);
    DrawCentered(mode.mode == GameModeType::Expedition
                     ? TextFormat("SEED %u", mode.runSeed)
                     : TextFormat(T("BASE %lld  x%.2f DIFF  x%.2f ASC  x%.2f RULES", "BASE %lld  x%.2f DIF  x%.2f ASC  x%.2f REGRAS"), score.baseScore,
                                  score.difficultyMultiplier, score.ascensionMultiplier,
                                  score.mutatorMultiplier), 349 - shift, 14, LIGHTGRAY);
    if (mode.mode == GameModeType::Survival)
        DrawCentered(TextFormat(T("MUTATORS  %s", "MODIFICADORES  %s"), MutatorSummary(config).c_str()), 372 - shift, 12, VIOLET);
    DrawCentered(T("BUILD", "CONFIGURAÇÃO"), 398 - shift, 17, GOLD);
    int buildY = 421 - shift;
    for (int index = 0; index < session.Loadout().WeaponCount(); ++index) {
        const WeaponInstance& weapon = session.Loadout().Weapons()[static_cast<std::size_t>(index)];
        const char* name = weapon.evolved != EvolvedWeaponType::None
                               ? EvolvedWeaponName(weapon.evolved)
                               : GetWeaponDefinition(weapon.type).name;
        const std::string levelText = weapon.evolved != EvolvedWeaponType::None
                                          ? "MAX" : "Lv" + std::to_string(weapon.level);
        DrawCentered(TextFormat("%s  %s", K(name), levelText.c_str()),
                     buildY, 15, RAYWHITE);
        buildY += 16;
    }
    const float centerX = GetScreenWidth() * 0.5f;
    const float actionY = std::min(590.0f, GetScreenHeight() - 62.0f);
    if (DrawButton({centerX - 300.0f, actionY, 270.0f, 55.0f}, T("RESTART", "REINICIAR")) ||
        IsKeyPressed(KEY_ENTER)) return MenuAction::Restart;
    if (DrawButton({centerX + 30.0f, actionY, 270.0f, 55.0f}, T("MAIN MENU", "MENU PRINCIPAL")))
        return MenuAction::MainMenu;
    return MenuAction::None;
}

void GameUI::DrawDebugOverlay(const GameSession& session, const ModeHUDData& mode) {
    const Vector2 position = session.GetPlayer().Position();
    const Rectangle panel{GetScreenWidth() - 355.0f, 62.0f, 330.0f, 590.0f};
    DrawRectangleRec(panel, Color{5, 8, 12, 215});
    DrawRectangleLinesEx(panel, 1.0f, Color{100, 230, 180, 220});
    DrawText(TextFormat("FPS: %d", GetFPS()), static_cast<int>(panel.x + 12),
             static_cast<int>(panel.y + 10), 18, GREEN);
    DrawText(TextFormat("%s A%d %s C%d", DifficultyName(mode.difficulty), mode.ascension,
                        mode.endless ? "ENDLESS" : "STANDARD", mode.endlessCycle),
             static_cast<int>(panel.x + 95), static_cast<int>(panel.y + 12), 12, GOLD);
    DrawText(TextFormat("Enemies Active: %d", session.Enemies().ActiveCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 36), 18, RAYWHITE);
    DrawText(TextFormat("Score %lld x%.2f", session.CurrentScore().finalScore, mode.scoreMultiplier),
             static_cast<int>(panel.x + 174), static_cast<int>(panel.y + 39), 12, GREEN);
    DrawText(TextFormat("Player Projectiles: %d",
                        session.Projectiles().ActiveCount(ProjectileOwner::Player)),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 62), 18, RAYWHITE);
    DrawText(TextFormat("M%d %s", mode.mutatorCount, ChallengeName(mode.challenge)),
             static_cast<int>(panel.x + 190), static_cast<int>(panel.y + 66), 11, VIOLET);
    DrawText(TextFormat("Enemy Projectiles: %d",
                        session.Projectiles().ActiveCount(ProjectileOwner::Enemy)),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 88), 18, RAYWHITE);
    DrawText(TextFormat("XP Orbs Active: %d", session.XPOrbs().ActiveCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 114), 18, RAYWHITE);
    DrawText(TextFormat("Player: %.0f, %.0f", position.x, position.y),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 140), 18, RAYWHITE);
    DrawText(TextFormat("Level: %d  W:%d/6  P:%d/6", session.GetPlayer().stats.level,
                        session.Loadout().WeaponCount(), session.Loadout().PassiveCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 166), 18, RAYWHITE);
    DrawText(TextFormat("DMG %.2f  CD %.2f  AREA %.2f", session.GetPlayer().stats.damageMultiplier,
                        session.GetPlayer().stats.cooldownMultiplier,
                        session.GetPlayer().stats.areaMultiplier),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 192), 17, RAYWHITE);
    DrawText(TextFormat("Luck %.2f  Armor %.1f", session.GetPlayer().stats.luck,
                        session.GetPlayer().stats.armor),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 218), 17, RAYWHITE);
    DrawText(TextFormat("Elites: %d  Pickups: %d  Particles: %d",
                        session.Enemies().EliteCount(), session.Pickups().ActiveCount(),
                        session.Particles().ActiveCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 244), 16, RAYWHITE);
    DrawText(TextFormat("Grid Cells: %d  Entries: %d", session.Enemies().GridCellCount(),
                        session.Enemies().GridEntryCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 270), 16, RAYWHITE);
    DrawText(TextFormat("Mode: %s  Seed: %u", GameModeName(mode.mode), mode.runSeed),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 296), 16, RAYWHITE);
    DrawText(TextFormat("Event: %s (%.1fs)", mode.eventName, mode.eventRemaining),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 322), 16, RAYWHITE);
    DrawText(TextFormat("Special: %s (%.1fs)", mode.specialWaveName, mode.specialWaveRemaining),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 348), 16, RAYWHITE);
    DrawText(TextFormat("Miniboss: %s  Threat %.0f  Var %d",
                        session.Enemies().ActiveMiniboss() ?
                            GetEnemyDefinition(session.Enemies().ActiveMiniboss()->type).name : "NONE",
                        mode.threatBudget, session.Enemies().VariantCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 374), 15, RAYWHITE);
    DrawText(TextFormat("%s %d %s | %.2fs x%d cap%d", mode.progressLabel, mode.progressIndex,
                        mode.progressName, mode.spawnInterval, mode.spawnCount, mode.enemyCap),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 402), 14, RAYWHITE);
    DrawText(TextFormat("Update %.2fms Draw %.2fms", session.UpdateMilliseconds(), session.DrawMilliseconds()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 427), 14, GREEN);
    DrawText(TextFormat("Boss: %s  phase %d", session.Bosses().IsActive()
                        ? session.Bosses().ActiveBoss().name : "none",
                        session.Bosses().IsActive() ? session.Bosses().ActiveBoss().phase : 0),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 454), 15, RAYWHITE);
    DrawText(TextFormat("Telegraphs %d  Queue %d  Chests %d", session.Bosses().TelegraphCount(),
                        mode.pendingEncounters, session.Chests().ActiveCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 480), 15, RAYWHITE);
    DrawText(TextFormat("Boss shots %d  Evolutions %d  Ready %d", session.Projectiles().ActiveCount(ProjectileOwner::Boss),
                        session.Loadout().EvolutionCount(), session.Loadout().EligibleEvolutionCount()),
             static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 506), 15, RAYWHITE);
    if (mode.mode == GameModeType::Expedition) {
        const AttackRequest& attack = session.CurrentAttackRequest();
        DrawText(TextFormat("Aim %.2f, %.2f  Route seed %u", attack.aimDirection.x,
                            attack.aimDirection.y, mode.runSeed),
                 static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 532), 13, GOLD);
        DrawText(TextFormat("LMB %.2fs  RMB %.2fs  Q swap", session.Loadout().SlotCooldown(0),
                            session.Loadout().SlotCooldown(1)),
                 static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 555), 13, GREEN);
    } else {
        DrawText(TextFormat("%s A%d %s C%d | M%d x%.2f", DifficultyName(mode.difficulty), mode.ascension,
                            mode.endless ? "ENDLESS" : ChallengeName(mode.challenge), mode.endlessCycle,
                            mode.mutatorCount, mode.scoreMultiplier),
                 static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 532), 13, GOLD);
        DrawText(TextFormat("Score %lld | G/H ITEMS", session.CurrentScore().finalScore),
                 static_cast<int>(panel.x + 12), static_cast<int>(panel.y + 555), 13, GREEN);
    }
    DrawText("F10 +1M Y +5M F11/F12/B N/C/M", static_cast<int>(panel.x + 12),
             static_cast<int>(panel.y + 576), 12, GRAY);
}
