# Game Mode Architecture

## Game State vs Game Mode

`GameState` descreve a tela ou fase da aplicação: `MainMenu`, `ModeSelection`, `Playing`,
`Paused`, `Settings`, `LevelUp`, `ChestReward`, `GameOver` e `Victory`. `GameModeType`
descreve as regras da run: `Survival` ou, futuramente, `Expedition`. Assim, uma partida
atual usa simultaneamente `GameState::Playing` e `GameModeType::Survival`.

## Estrutura

```text
                         Application
                              │
                      GameModeManager
                              │
                 ┌────────────┴────────────┐
                 │                         │
          SurvivalMode              ExpeditionMode
                 │                    [Stage 11+]
          WaveDirector
                 │
        Boss scheduling
                 │
   Future Survival systems
        [Stages 8–10]
                 │
                 └────────────┬────────────
                              │
                       Shared Systems
                              │
       ┌──────────────────────┼──────────────────────┐
       │                      │                      │
     Player              Combat / XP          Enemies / Bosses
 Weapons / Passives      Loot / Chests         Projectiles / Grid
     Evolutions          VFX / Audio
```

`GameModeManager` seleciona e ativa o modo, executa seu hook de progressão, atualiza a
simulação compartilhada e consolida `RunContext`, `RunOutcome` e `RunResult`. A abstração
ocorre uma vez por frame, nunca por entidade.

## Shared Systems

`GameSession` é o contêiner dos sistemas reutilizáveis: Player, stats, armas, passivas,
evoluções, inimigos, bosses, projéteis, áreas, XP, loot, pickups, chests, Spatial Grid,
partículas, floating text e estatísticas. Esses componentes não incluem `GameModeType` e
não sabem por que um inimigo ou boss foi solicitado.

`BossManager` executa um encontro solicitado e sua IA. `EnemyManager` cria, atualiza e
remove inimigos. Chests e evolução são compartilhados; o Survival apenas concede a
oportunidade atual após bosses intermediários.

## Survival Mode

`SurvivalMode` é o único modo disponível. Ele possui o `WaveDirector`, avança o timer com
o limite configurado da run, agenda os bosses de 05:00, 10:00 e 15:00, reduz a pressão de
adds durante bosses, concede os chests atuais e declara vitória depois da morte do boss
final. Dano, IA, armas e física permanecem em sistemas compartilhados.

```text
SurvivalMode
├── WaveDirector
├── Boss scheduling
├── SurvivalEvents       [Stage 8]
├── SurvivalContent      [Stages 8–9]
└── SurvivalModifiers    [Stage 10]
```

Os três últimos itens são pontos de extensão documentados, não classes vazias.

## Expedition Mode

`ExpeditionMode` está disponível na seleção e delega sua progressão linear ao
`ExpeditionDirector`. O MVP solicita os managers compartilhados sem criar versões
paralelas de Player, armas, inimigos ou bosses. RouteManager, branching, salas
procedurais e eventos de Expedition continuam reservados para etapas futuras.

## Run Lifecycle

```text
MainMenu → ModeSelection → ActivateMode
         → ResetSharedRunSystems
         → mode.Reset / OnEnter / OnRunStart
         → Playing
         → mode progression + shared update
         → RunOutcome
         → Restart current mode ou ExitActiveMode → MainMenu
```

Restart preserva o tipo do modo e limpa estado compartilhado e específico. Settings
pertence à `Application` e não participa desse reset.

## Victory / Defeat

`RunOutcome::Defeat` vem da morte do Player e tem prioridade. A vitória é perguntada ao
modo ativo; no Survival ela ocorre somente após o Void Herald Ascended morrer. O núcleo
recebe o outcome sem conhecer a regra de 15 minutos. `RunResult` captura modo, resultado,
tempo, nível, kills, elites, bosses, dano, chests e evoluções.

## Mode-specific Data e HUD

`ModeHUDData` é um snapshot pequeno fornecido pelo modo. O HUD compartilhado desenha HP,
XP, level, loadout, kills e boss; os dados do Survival acrescentam timer, nome/índice da
wave e fila de encontros. Expedition poderá fornecer stage/room/objective sem duplicar o
HUD inteiro ou acoplar a UI ao seu futuro director.

## Future Expansion Points

- Stage 8 pode estender a orquestração do Survival com special waves, events, minibosses
  e conteúdo sem alterar Player/EnemyManager.
- Stage 9 amplia catálogos compartilhados de characters, weapons, passives e evolutions.
- Stage 10 adicionará configuração de dificuldade, challenges, mutators, ascension e
  endless no nível do Survival, sem checks por entidade. Um futuro `SurvivalRunConfig`
  poderá reunir duration, difficulty, scaling, mutators e endless.
- Stage 11 introduziu o primeiro gameplay real do Expedition e seu director.

## Roadmap

```text
Stage 7   Multi-mode architecture
Stage 8   Survival Expansion I
Stage 9   Survival Expansion II
Stage 10  Survival Endgame & Challenges
Stage 11  Expedition MVP
Stage 12  Expedition Route System
Stage 13  Expedition Events & Rooms
Stage 14  Expedition Acts & Biomes
Stage 15  Meta Progression
Stage 16  Final Expansion / Endgame
```

Stages 8–10 continue development of Survival Mode.

Stage 11 began actual Expedition gameplay development.

That architectural placeholder is now replaced by the playable Expedition MVP below.
# Survival Expansion I boundaries

The event manager, special-wave schedule and miniboss schedule are subordinate to
`SurvivalMode`; none is initialized by Expedition. Survival
continues in the same arena with its original 05:00, 10:00 and 15:00 bosses. Event
starts are rejected during bosses, minibosses, special waves, queued bosses and the
final safety window before each boss.

Scheduled miniboss windows are approximately 03:15, 07:45 and 12:35. If another
encounter owns the arena, the miniboss remains due and starts later. It never grants
an evolution chest; it drops large XP plus a guaranteed useful pickup.

## Stage 9 scope

Stage 9 expanded Survival content and build diversity without adding mode-specific gameplay systems. `RunContext` and `RunResult` carry `CharacterId`, so Restart, Game Over and Victory consistently identify the selected survivor. Expedition gameplay was subsequently added in Stage 11.

## Stage 10 scope

Survival now has a dedicated setup between character selection and run activation. Quick Start selects Normal/A0/Standard; custom setup selects Normal, Hard or Nightmare, Ascension 0–10, up to three compatible mutators, one of six locked challenges, or Endless. One resolved `SurvivalRunConfig` is copied into `RunContext`, `GameSession` and `RunResult`; Restart preserves it and menu exit clears runtime state.

Standard Survival still schedules bosses at 05:00, 10:00 and 15:00 and wins only when the final boss dies. Endless suppresses that victory, continues waves/events/minibosses, and schedules a rotating boss every five minutes starting at 20:00. It ends on death or menu exit.

## Expedition MVP (Stage 11)

Expedition is playable after character selection. It owns five handcrafted linear
stages: two opening combats, an elite stage, a denser mixed encounter and a Void Herald
boss stage. It deliberately has no route graph, branches, shops, acts, procedural rooms
or Survival events.

Each stage carries its map mask, spawn groups, objective and encounter class. Completion
first resolves accumulated level-ups, then presents three valid stage rewards. Selecting
one opens the exit portal; crossing it fades into the next stage. The HUD shows stage and
objective instead of Survival waves. Restart returns to Stage 1 with a clean build and
the same run seed and survivor.
# Diferença fundamental de combate (Etapa 12)

- `SURVIVAL`: bullet-heaven automático, busca de alvos e até seis armas simultâneas.
- `EXPEDITION`: mira manual pelo mouse, duas armas ativas (LMB/RMB), swap com Q e seleção de rota.

As duas modalidades usam as mesmas definições, instâncias, projéteis, evoluções e passivas. Apenas
a política de input e `LoadoutRules` muda por modo, evitando dois sistemas de armas divergentes.
