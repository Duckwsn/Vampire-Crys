# Arquitetura da versão 0.1.0

## Estrutura

- `src/core`: loop da aplicação e estados de alto nível.
- `src/gameplay`: sessão da partida e regras que coordenam os sistemas.
- `src/game_modes`: seleção, ciclo de vida e regras de progressão por modo.
- `src/entities`: dados e comportamento local do jogador.
- `src/systems`: pools, grid espacial, waves, armas, bosses, pickups, VFX e áudio.
- `src/ui`: menus, HUD, level-up, game over e debug.
- `include`: interfaces públicas espelhando os módulos de `src`.

`VisualStyle` centraliza o desenho procedural de personagens, inimigos, bosses, projéteis, pickups, ambiente e ícones. `Draw` permanece separado de `Update`; arte nunca determina colisão, velocidade, dano ou cooldown.

## Game loop e estados

`Application` mantém a janela e alterna entre `MainMenu`, `ModeSelection`, `Playing`, `Paused`, `Settings`, `LevelUp`, `ChestReward`, `GameOver` e `Victory`. Entrada, atualização e renderização são executadas uma vez por frame com `delta time` limitado para evitar saltos grandes após interrupções.

A prioridade após cada update é `Defeat > Victory > LevelUp > ChestReward`. Portanto, morte simultânea à derrota do boss final resulta deterministicamente em Game Over. Pausa e modais não chamam `GameModeManager::UpdateActiveMode`, congelando modo, waves, ataques, telegraphs e timers. O loop limita delta a 0,05s.

`GameSession` contém o estado compartilhado da partida. Durante `LevelUp` e `ChestReward`, a simulação fica parada. Level-ups pendentes têm prioridade sobre chest para impedir dois modais simultâneos. `ResetSharedRunSystems` recria Player, pools, loadout, estatísticas e transientes; Settings permanece fora da sessão.

## Game Mode Layer

`GameState` representa telas e modais; `GameModeType` representa as regras da run. `GameModeManager` ativa uma implementação de `IGameMode`, coordena reset/start/update/end/exit e produz `RunContext`, `RunOutcome` e `RunResult`.

`SurvivalMode` contém `WaveDirector`, limite configurado do timer, milestones 05:00/10:00/15:00, pressão de adds durante boss, recompensa de boss intermediário e condição de vitória. `ExpeditionMode` contém seu próprio diretor de encontros e o DAG de rota por seed. Sistemas compartilhados não dependem de nenhum dos dois.

## Entidades e pools

Inimigos, projéteis e orbes de XP vivem em vetores de capacidade fixa reservada. Cada item tem uma flag `active`; spawn reutiliza um slot inativo e morte/expiração apenas devolve o slot ao pool. Não há `new`/`delete` por entidade durante a partida.

## Arma e auto-target

`WeaponManager` mantém até seis `WeaponInstance` e seis passivas. Definições e progressões ficam separadas em `Definitions`; o manager despacha somente os seis comportamentos atuais. A aquisição de alvos é centralizada em `EnemyManager::FindNearest` e usa a `SpatialGrid` sem acoplar armas à sua implementação.

`AreaEffectManager` mantém um pool de áreas com ticks por delta time. Flame Ring e explosões do Thunder Cannon compartilham esse mecanismo. `ProjectileManager` mantém um pool único com ownership explícito para projéteis do jogador e inimigos.

## Inimigos

`Enemy` carrega `EnemyType`, dados de runtime e estado leve do comportamento. `EnemyManager` mantém o pool, perseguição, Cultist ranged, Bomber telegraph, separação limitada, dano e desenho; o `WaveDirector` decide quando e o que nasce. Stats dos cinco tipos vêm de definições centralizadas.

## XP e level-up

`XPOrbManager` mantém o pool, atração e coleta. `GameSession` adiciona XP, aplica a fórmula centralizada `XPRequiredForLevel` e conta níveis pendentes, preservando excedentes. Cada nível pendente gera três opções válidas e sem duplicatas entre novas armas, upgrades de armas, novas passivas e upgrades de passivas.

## Colisões

Colisões usam círculos: projétil/área/orbital contra inimigo; contato/projétil/explosão inimiga contra jogador; jogador contra orbe de XP. Mortes e explosões são encaminhadas por filas fixas de eventos. A `SpatialGrid` reduz os candidatos e cada sistema ainda faz o teste geométrico exato.

## Extensibilidade

Novos `EnemyType`, armas, upgrades e fases podem ser adicionados pelas interfaces existentes sem mudar o ciclo de vida da sessão. A versão 0.1.0 mantém catálogos compactos e value-types, sem ECS ou renderer próprio.

## Estrutura da run — Etapa 3

`WaveDirector` lê `config/waves.json`, seleciona a fase pelo timer e solicita spawns ao `EnemyManager`. Ele centraliza intervalo, quantidade, cap, pesos, limites de Cultist/Bomber, scaling e chance de elite. Configuração ausente ou inválida usa defaults seguros e gera log.

`SpatialGrid` pertence ao `EnemyManager` e é reconstruída após movimento. Seus índices estáveis alimentam nearest target, projéteis, áreas, orbitais e separação. A grid reduz candidatos; colisões finais continuam geométricas.

Elites reutilizam os cinco tipos com flag e multiplicadores. Mortes passam por `LootSystem`, que pode solicitar um item pooled ao `PickupManager`. Health cura, Magnet acelera todos os XP Orbs e Bomb aplica dano alto aos inimigos visíveis consultados pela grid.

`ParticleManager` fornece partículas discretas pooled para mortes e coletas. `RunStatistics` registra tempo, nível, kills, elites, XP, dano e pickups. A condição temporária de Victory aos 900 segundos foi removida na Etapa 4.

## Boss System — Etapa 4

`BossManager` possui um único `Boss` ativo e um controller separado de `EnemyManager`. A máquina de estados segue `Spawn → Move → AttackSelection → Telegraph → AttackExecution → Recovery`, com `Death` e transição de fase por HP. Bosses têm query específica combinada no targeting; projéteis, áreas e orbitais chamam o mesmo caminho de dano, sem inserir um boss no pool/grid de inimigos comuns. Como o dano do boss não aceita deslocamento, a resistência a knockback é efetivamente 100%.

`TelegraphManager` mantém 48 slots reutilizáveis para círculos, cones e linhas grossas. A mesma geometria desenhada faz os testes de colisão: circle/circle, circle/cone com tolerância do raio do jogador e distância a segmento para beams. Ataques perigosos só são ativados depois do warning e usam ticks temporizados.

No `SurvivalMode`, os milestones 05:00, 10:00 e 15:00 são flags únicas. Encontros atrasados permanecem numa fila ordenada; um novo boss só nasce após a morte e um pequeno intervalo. Durante o encontro, `WaveDirector` usa 20% da frequência, um spawn por ciclo e cap de 30 adds; excedentes preexistentes são removidos sem XP/drop.

Flame Wyrm alterna Fire Breath e Flame Pools. Void Herald alterna Radial Barrage e Void Beams. Void Herald Ascended reutiliza o controller com stats, anéis, beams, recovery e threshold de fase próprios. Projéteis possuem owner `Boss`; morte limpa projéteis, telegraphs e hazards daquele encontro. O evento de morte é compartilhado; `SurvivalMode` concede chest aos intermediários e define `Victory` para o Ascended.

`ChestManager` mantém quatro slots e abre por contato. `GameSession` monta até três opções: primeiro evoluções elegíveis, depois upgrades de itens possuídos, e por fim cura+XP quando o build está máximo. `WeaponEvolutionDefinition` centraliza base, passiva e forma final. `WeaponInstance::evolved` substitui o comportamento no mesmo slot e, como a arma-base já está Lv8, ela deixa naturalmente o catálogo normal de level-up.

## Apresentação — Etapa 5

A animação procedural deriva de estado explícito (`moving`, facing persistente, hit flash e fase/ataque de boss) e tempo visual; não existe slicing de spritesheet. `ParticleManager` possui 1.600 slots e quatro formas; efeitos importantes podem substituir cosméticos de baixa prioridade. `FloatingTextManager` agrega hits próximos. `ScreenShakeManager` fornece offset com decay à câmera suavizada e intensidade configurável.

`AudioManager` pertence à `Application`: sintetiza recursos após `InitAudioDevice`, limita repetição por cue, varia pitch e faz fade entre quatro estados musicais. `GameSession` apenas emite cues. O shutdown descarrega sons antes do dispositivo.

`GameSettings` lê e grava JSON junto ao executável. A gravação usa `.tmp`, preserva o anterior em `.bak` durante a troca e restaura o backup se a finalização falhar. Settings pertence à aplicação e não participa de `GameSession::Reset`.

O pipeline é ambiente → áreas/telegraphs/boss → pickups/XP → inimigos → jogador/armas/projéteis → partículas/texto → HUD/modais/debug. UI usa tela; mundo e hitboxes usam `Camera2D`. F2 desenha colisões sem alterar a simulação.
# Stage 8 Survival orchestration

`SurvivalMode` remains the only owner of Survival scheduling. It coordinates boss
milestones, miniboss windows and `SurvivalEventManager`, then passes transient
modifiers into `WaveDirector`. Event values are queried per frame and never multiply
stored enemy/player stats, so ending an event or restarting cannot leave residue.

`WaveDirector` executes both normal composition and five bounded special-wave
patterns. Every special group spends a threat budget before spawning through the
existing `EnemyManager` pool. Events, special waves, minibosses and bosses are
mutually gated; due boss encounters have scheduling priority.

New common enemies, variants, Bone Minions and minibosses all use `EnemyManager`, its
fixed pool, spatial grid, collision pipeline and weapon targeting. `SpawnSource`
marks director, special-wave, summon, Elite Hunt and miniboss entities for rewards.
Summons have per-necromancer and global caps and cannot roll normal loot.

Phase Dash is player-owned input/state. `GameSession` consumes its one-frame start
signal for pooled VFX while ordinary damage paths continue to call `TakeDamage`,
which honors the dash invulnerability window.

## Stage 9 — characters, traits and status

`CharacterDefinition` is a compact immutable catalog keyed by `CharacterId`. It owns display metadata, starting weapon, base multipliers and one `CharacterTrait`. Selection flows through `GameState::CharacterSelection`; `GameModeManager` stores the chosen id in `RunContext`, and `GameSession::ResetSharedRunSystems` applies it once to the shared `Player` and `WeaponManager`. Restart keeps the id but rebuilds all runtime stats and pools, preventing modifier leakage.

Traits are small player/session hooks: healthy-target damage is supplied to `EnemyManager`, Pyromancer consumes the manager's per-frame AoE hit count, Sentinel owns an unharmed timer, and Occultist is fully expressed by initial stats. There are no character subclasses or parallel damage pipelines.

Slow is stored per `Enemy`/`Boss` as remaining duration plus movement multiplier. Reapplication keeps the strongest multiplier and longest duration; expiration restores 1.0. Normal enemies clamp at 55% speed, minibosses receive 45% strength/55% duration, and bosses clamp at 88% speed/40% duration. Pull uses enemy knockback velocity, is reduced to 20% on minibosses and has no boss path.

Soul Scythe and chain attacks query `EnemyManager` only when firing. Frost and Blood use pooled projectiles; Blood stores a stable pool index for bounded steering. Gravity Well uses pooled `AreaEffect` ticks and one Spatial Grid query per tick. All ten base weapons still share the same six-slot `WeaponManager`.

## Stage 10 — Survival endgame configuration

`SurvivalRunConfig` is a value object carried by `Application`, `RunContext`, `GameSession` and `RunResult`. `ResolveSurvivalConfig` sanitizes enum values, clamps Ascension and replaces editable rules with a challenge preset. `BuildRunModifiers` is the only difficulty/Ascension/mutator pipeline. Runtime systems consume its specific values; they do not permanently multiply stored definitions.

`SurvivalMode` remains the scheduling owner. It combines event modifiers with resolved run modifiers before calling `WaveDirector`, applies boss/miniboss scaling and extends the milestone queue for Endless. Normal 15-minute victory is unchanged; Endless turns the final death into a chest and schedules rotating bosses every five minutes. Pools, collisions and shared entities remain mode-agnostic.

Score is derived from `RunStatistics` and the resolved config, so it has no independent mutable counter to leak across restarts. Settings and run config have separate lifetimes and persistence boundaries.

## Localization and responsive setup

`GameSettings` persists `GameLanguage` independently from run configuration. The small presentation-only `Localization` adapter resolves English/PT-BR labels and catalog text; gameplay definitions, IDs and save-compatible enum values remain language-neutral. `Application` applies the selected language globally before drawing, and Settings can switch it immediately.

Dense setup/settings screens suppress the decorative menu emblem instead of drawing interactive controls over it. Survival Setup keeps its 540p-safe base layout and adds a bounded vertical offset on taller windows, while selector arrows and unavailable mutator states are explicit.

## Expedition MVP — Etapa 11

`ExpeditionMode` possui um `ExpeditionDirector`, separado de `WaveDirector` e da agenda
de eventos do Survival. O diretor executa cinco definições lineares: intro, grupos de
encontro, level-ups, reward, saída, transição e próxima fase. O quinto estágio inicia
o boss compartilhado e conclui a run pela interface comum de `RunOutcome`.

`ExpeditionMap` interpreta máscaras de tiles autorais e valida conectividade por flood
fill. Seu contrato opcional `WorldNavigation` atende Player, EnemyManager, BossManager e
câmera. Na Expedition ele fornece colisão circular com slide, posição segura, limites
de câmera e flow field por célula; no Survival o comportamento anterior permanece.

`ResetStageCombatSystems` limpa entidades e hazards transitórios entre fases, preservando
PlayerStats, loadout, XP, nível, timer e estatísticas. O reward pool respeita slots e
níveis máximos e expõe evoluções elegíveis.
# Extensão da Etapa 12

O fluxo de combate é `input -> AttackRequest -> WeaponManager`: Survival chama a atualização
automática existente; Expedition chama a atualização manual com direção e posição de mira. O limite
de slots é uma regra configurada pelo modo (6/2), não uma duplicação do inventário.

`ExpeditionDirector` mantém um `ExpeditionRouteGraph` leve composto por `ExpeditionRouteNode`.
O gerador determinístico usa a seed da run, cria sete camadas, conecta somente camadas adjacentes e
valida endpoints, conectividade, ausência de dead ends e presença de escolhas Elite/Reward. O estado
do grafo e o caminho escolhido persistem entre arenas. `ExpeditionMapDefinition` continua sendo a
fonte dos layouts autorais; a rota seleciona encontros e mapas compatíveis do catálogo.
