# Decisões técnicas

- C++17, CMake e Raylib 5.5 fixada por tag para builds reproduzíveis.
- Raylib é encontrada localmente quando disponível; caso contrário, CMake usa `FetchContent`.
- Resolução base de 1280x720 e arena limitada maior que a tela, acompanhada por `Camera2D`.
- Composição simples em vez de uma hierarquia profunda de entidades: os dados são pequenos e os managers representam os limites naturais dos sistemas.
- Pools com capacidade fixa e slots ativos evitam alocações por frame. Se um pool lotar, o spawn daquele item é ignorado com segurança.
- Busca linear para alvo e colisões é suficiente para a carga do MVP e está encapsulada para troca futura.
- Valores de balanceamento ficam em `gameplay/Balance.h`; nenhuma configuração externa é necessária nesta etapa.
- Upgrades do MVP são um enum pequeno com aplicação direta. Um catálogo orientado a dados fica reservado para etapas com mais conteúdo.
- O timer avança apenas durante `Playing`, pois a tela de level-up pausa integralmente a partida.

## Etapa 2

- O controlador isolado `ArcBoltWeapon` será substituído por `WeaponManager`. A mudança é necessária porque cooldowns, níveis, orbitais e ataques de área precisam compartilhar loadout e modificadores globais; o comportamento original do Arc Bolt será preservado como um dos seis dispatches.
- Definições estáticas de armas, passivas e inimigos ficam centralizadas em `Definitions`. Balanceamento permanece em C++ nesta etapa para evitar um sistema de dados externo prematuro.
- `ProjectileManager` continuará sendo um único pool, agora com `ProjectileOwner`. Contagens e colisões permanecem logicamente separadas por owner sem duplicar infraestrutura.
- Mortes e explosões usam filas fixas de eventos. Isso desacopla a aplicação de dano da geração de XP/efeitos sem alocar memória por hit.
- `AreaEffectManager` será o mecanismo comum de áreas do jogador, usado por Flame Ring e explosões do Thunder Cannon.
- Inimigos continuam dados compactos com `EnemyType` e dispatch de comportamento no manager. Para apenas cinco tipos, isso é mais simples que uma hierarquia polimórfica e mantém o pool contíguo.
- Separação local verifica no máximo oito slots posteriores por inimigo. É uma mitigação temporária e limitada até a Spatial Grid da Etapa 3.
- Armadura usa `max(1, dano recebido - armor)`. Regeneração é HP por segundo. Aumento de HP do Vital Heart também cura a mesma quantidade.
- Raridade é propriedade da definição e altera o peso de seleção: Common 60, Uncommon 25, Rare 12, Epic 3. Cada ponto de Luck reduz Common e aumenta Rare/Epic com clamps, sem alterar diretamente os stats da opção.

## Etapa 3

- `EnemyManager` passa a possuir uma grade espacial densa de 32×32 células (128 px) porque a arena já é fixa. Isso evita hashing/alocações e mantém índices do pool estáveis.
- Waves vêm de `config/waves.json`; `nlohmann/json` é uma dependência header-only fixada por tag. Configuração inválida gera warning e defaults seguros completos até 15 minutos.
- Spawn sai do `EnemyManager` e fica exclusivamente no `WaveDirector`, preservando o manager como armazenamento/IA/consulta.
- Elites reutilizam `Enemy` com flag e multiplicadores; slots são zerados integralmente no spawn para impedir vazamento ao reutilizar o pool.
- Loot especial é decidido por `LootSystem` após evento de morte e materializado em `PickupManager`; XP continua garantido e separado.
- A conclusão em 15:00 usa `Victory` provisório. Bosses da Etapa 4 poderão substituir o gatilho sem alterar o resumo da run.

## Etapa 4

- A decisão provisória de Victory da Etapa 3 foi substituída: 15:00 apenas agenda `VoidHeraldAscended`; somente sua morte ativa `Victory`.
- Bosses usam controller e query próprios, combinados com a `SpatialGrid` no ponto de targeting/colisão. Isso evita contaminar `EnemyManager` com estados especiais sem duplicar os seis caminhos de dano.
- Milestones usam três flags `due/completed` ordenadas. Se um boss atrasar, os próximos ficam pendentes e entram um por vez após 1,5s.
- Durante bosses, waves operam a 20% da frequência e cap 30. Inimigos acima do cap são desativados sem recompensa para não transformar limpeza técnica em XP.
- Telegraphs e hazards compartilham formas e colisões no `TelegraphManager`; morte do boss reseta o pool e remove todos os projéteis com owner `Boss`.
- Evolução é exclusiva de chest para manter o evento de boss relevante. Exige arma Lv8 e apenas posse da passiva em Lv1+, nunca passiva máxima.
- A forma evoluída é uma flag no `WeaponInstance`, não outro `WeaponType`. Assim ocupa o mesmo slot, preserva a API de seis armas-base e não entra no level-up comum.
- Quando Level Up e chest ficam pendentes juntos, todos os Level Ups são resolvidos primeiro; o chest abre imediatamente depois, com a simulação ainda pausada.

## Etapa 5

- A solicitação explícita de formas geométricas substitui spritesheets/pixel art. `VisualStyle` oferece uma linguagem coerente de polígonos compostos, outline e energia; nenhum PNG falso foi criado.
- O player usa HSV contínuo e conserva outline branco/sombra para permanecer identificável em qualquer hue.
- Animações são paramétricas e orientadas pelo estado existente; apresentação continua separada do gameplay.
- Não foi adotada resolução virtual: UI e câmera usam dimensões reais, janela mínima 960×540 e anchors, evitando letterbox e remapeamento de mouse.
- Hit flash usa cor, sem shader por entidade. Partículas têm cap/prioridades e damage text agrega hits rápidos.
- Áudio é sintetizado em memória. Cooldown por cue evita sobreposição; pitch varia de 0,96 a 1,04 e música troca com fade.
- Até a Etapa 5, Settings ficava apenas em memória.

## Etapa 6

- O release interno permanece em 0.1.0. Cheats são gated por `GAME_DEBUG`, definido apenas em Debug.
- Config é resolvido a partir do diretório do executável; nenhum caminho absoluto do checkout é embutido.
- Settings ganhou persistência JSON mínima e transacional, sem introduzir metaprogressão fora de escopo.
- Stats do player são sanitizados em limites defensivos contra NaN, raio negativo e cooldown inválido.
- A prioridade final é `GameOver > Victory > LevelUp > ChestReward`; morte ganha de derrota simultânea do boss.
- O balanceamento numérico foi preservado: matriz de armas/evoluções, soak e stress não apontaram arma ou fase quebrada, e mudanças sem telemetria humana seriam especulativas.

## Hotfix pré-Etapa 7

- `ESC` é amostrado somente em `Application::Update` e convertido por uma tabela pura de ações; UI de desenho não consulta mais essa tecla. Toda transição retorna antes do update específico do estado de destino.
- O projeto continua sem resolução virtual: a borda exposta vinha da cobertura procedural fixa de 1520×940. A cobertura agora acompanha a janela real com margem, preservando proporção e coordenadas do mouse.
- A linha sólida da arena foi trocada por 64 px de transição para o tom externo; Playing/Paused recebem ainda um fade de composição discreto de 48 px, antes do HUD e do overlay de pause.

## Etapa 7

- A abstração `IGameMode` existe no nível de uma run/frame para separar regras de progressão sem adicionar dispatch virtual por entidade. `GameModeManager` mantém implementações por valor e não usa framework de DI ou plugins.
- Survival permanece o modo primário e integralmente funcional. Etapas 8–10 continuarão expandindo Survival antes de qualquer gameplay de Expedition, que começa somente na Etapa 11.
- `SurvivalMode` passou a possuir WaveDirector, timer limitado, agenda dos três bosses, regra de chest intermediário e vitória final. `BossManager` apenas executa o encontro solicitado; `GameSession` apenas atualiza sistemas compartilhados.
- Player, armas, passivas, evoluções, inimigos, bosses, projéteis, XP, loot, pickups, chests, Spatial Grid, VFX e áudio continuam sem dependência de modo.
- `ExpeditionMode` é intencionalmente indisponível. O card e o enum provam o ponto de extensão, enquanto a recusa de ativação impede uma tela vazia ou Survival iniciado por engano.
- `RunOutcome` remove do núcleo a interpretação de “boss final aos 15 minutos”; `RunResult` captura somente estatísticas comuns aos modos.
# Stage 8 decisions

- Events are deterministic from a displayed per-run seed, unique per run, and use a
  Telegraph/Active/Ending/Cooldown lifecycle.
- Blood Moon uses a 0.72 spawn interval, 1.08 live speed and 1.5 elite-chance factor;
  The Swarm forces roughly 82% Swarmer spawns; Elite Hunt reduces normal pressure to
  72% while spawning three marked elites.
- Special waves remain inside `WaveDirector`; a separate spawning engine would have
  duplicated caps, safe-position rules and difficulty scaling.
- Minibosses are special pooled enemies rather than reduced bosses. This lets every
  existing weapon and area effect hit them without parallel collision code.
- Variants are applied once at spawn (Frenzied, Armored, Empowered) and are never
  promoted to elites or accumulated over time.
- Stage 5's geometric procedural direction remains authoritative: Stage 8 adds no
  bitmap enemy assets and extends the established shape language instead.

## Stage 9 decisions

- Characters share `Player` because movement, collision, dash, damage and animation state are identical; `CharacterDefinition` changes only data and lightweight trait hooks.
- Every character can roll the same weapon/passive catalog. The starting weapon creates identity without shrinking build diversity or introducing character-specific duplicate pools.
- Limits remain six weapons and six passives, with levels 8/5. Ten catalog entries increase choice pressure while preserving the HUD, chest rules and meaningful slot tradeoffs.
- Traits deliberately have one bounded trigger or a transparent base-stat tradeoff. They do not add a second active ability, skill tree or persistent progression.
- Status scope is intentionally limited to Slow and gravity pull. Both reuse entity fields and existing motion/tick paths; a generalized buff framework would be premature for two effects.
- The established geometric art direction overrides bitmap placeholders: portraits, weapon symbols and VFX are procedural original code with no external licensing burden.
- Blood Needle homing never performs a spatial query per projectile; targets are distributed once per volley and stored as stable pool indices. Gravity performs queries only on scheduled area ticks.

## Stage 10 decisions

- Difficulty, Ascension, mutators and challenges are data plus one explicit modifier pipeline; a generic rule engine would obscure the few special composition behaviors.
- Challenges resolve to locked Survival configs. They are not modes and cannot activate Expedition code.
- A run config is immutable after Start. Restart reuses it; New Run returns through setup. Audio/fullscreen settings remain separately persisted.
- Hyper Horde and Titanic are incompatible, and selection is capped at three for readable results and balance.
- Endless reuses the three existing bosses in a deterministic rotation. No Stage 11 content or duplicate boss controller was introduced.
- Cycle growth uses soft caps instead of unbounded linear speed/pressure. Existing object pools and encounter caps remain the hard memory boundary.
- Score is informational only. It grants no currency, unlock or permanent power, and Necromancer summons do not count as eligible kills.
- Local best-score persistence and an explicit Abandon result remain optional; leaving through the existing menu path clears the run.

## Localization hotfix

- Language is an application setting (`en` or `pt-BR`), not part of `SurvivalRunConfig`, so changing presentation cannot alter or invalidate a run.
- Gameplay catalogs keep one canonical English data source. A UI adapter translates known names/descriptions and generated upgrade terminology at draw time, avoiding duplicated combat definitions.
- English remains the safe default for old or malformed settings files. The selected language is saved when leaving Settings and during normal shutdown.

## Etapa 11 — Expedition MVP

- A sequência fixa de cinco fases foi a decisão da Etapa 11 e foi substituída na Etapa 12
  por um DAG determinístico de sete camadas com branching e seed reproduzível.
- Mapas usam máscaras de tiles autorais, editáveis e validadas por flood fill.
- `WorldNavigation` é opcional e compartilhado pelos sistemas móveis, preservando o
  comportamento do Survival e evitando checks de modo espalhados.
- Um flow field por célula orienta perseguidores; inimigos ranged preservam recuo e
  movimento lateral, com todos os deslocamentos corrigidos para uma posição navegável.
- Rewards reutilizam upgrades/evoluções do loadout, mas oferecem três opções próprias,
  cura parcial ou XP. Chests do Survival mantêm o comportamento anterior.
- O boss final reutiliza o controller existente; encontro e vitória pertencem apenas ao
  `ExpeditionDirector`.
# Decisões da Etapa 12

- Expedition usa mira manual para criar tensão entre mirar, mover e esquivar; Survival mantém o
  auto-attack para preservar sua identidade e regressões de gameplay.
- Duas armas dão papéis claros a LMB/RMB e tornam cada escolha legível. Substituição existe para que
  recompensas novas continuem relevantes sem expandir o limite.
- O grafo de rota é procedural e reproduzível por seed para variedade e depuração; arenas permanecem
  autorais para garantir navegação, colisões e encontros justos.
- Grave Warden foi redesenhado por comportamento (chase, charge, slam, shockwave e recovery), não
  por multiplicação extrema de vida.
