# Final Audit

Data da auditoria: 2026-09-10. Versão alvo: 0.1.0, release interno.

## Critical

Nenhum crash, undefined behavior evidente ou fluxo principal quebrado foi encontrado no baseline Release.

## High

- **FIXED — Portabilidade do config:** `waves.json` é resolvido a partir de `GetApplicationDirectory`; o pacote foi executado fora do checkout de fontes.
- **FIXED — Debug em Release:** entrada de cheats e visualizações está sob `GAME_DEBUG`, definido apenas na configuração Debug.
- **FIXED — Build Debug MSVC:** `/FS` foi adicionado e o build Debug serial passou com testes.

## Medium

- **FIXED — Settings:** JSON é persistido por arquivo temporário/backup; corrupção mantém defaults e foi testada.
- **FIXED — Config validation:** ranges, duração, caps e pesos são validados; erro usa defaults internos.
- **FIXED — Stat safety:** stats são sanitizados contra NaN, infinidade e ranges inválidos.
- **FIXED — Packaging:** componente `Runtime` gera pacote enxuto em `release/game/`.
- **FIXED — UI mínima:** menu, Settings e Victory foram reposicionados para não cortar ações em 960×540.

## Low

- **FIXED — Overlay:** frame time foi adicionado; menu mostra versão 0.1.0.
- **FIXED — Documentation:** fluxo, persistência, debug, prioridade de estados e packaging foram sincronizados.

## Performance

Clean Release passou 500 inimigos/250 projéteis/500 XP/1.000 partículas e cenário de boss. Hot paths usam pools e Spatial Grid; `PrepareUpgradeChoices` aloca apenas durante pausa de level-up.

## Memory / Resource Management

Entidades são value-types em pools pré-dimensionados. Spawns sobrescrevem slots com `{}` antes de ativar, evitando vazamento de estado. Sons são carregados uma vez e descarregados antes de `CloseAudioDevice`. Não há `new/delete/malloc/free` manual no projeto.

## Gameplay

Testes existentes cobrem armas, passivas, elites, filas de level-up, milestones, bosses, chests e seis evoluções. `GameModeManager` consolida `Defeat > Victory`; a `Application` mantém LevelUp antes de ChestReward.

## UI

Estados MainMenu, ModeSelection, Playing, Paused, Settings, LevelUp, ChestReward, GameOver e Victory existem. Survival está habilitado e Expedition aparece bloqueado; auditoria visual automatizada não está disponível nesta máquina.

## Audio

Buffers procedurais têm owner único, throttling e transição musical. Volumes persistem em Settings.

## Assets

Direção intencionalmente procedural: 0 spritesheets e 0 imagens. Validação de grade/metadata não se aplica; não existem falsos assets.

## Documentation

README, arquitetura, decisões, balanceamento, performance, assets e esta auditoria refletem a versão 0.1.0.

## Build / Packaging

Baseline: Release PASS; Debug inicialmente falhou por C1041. Final: Debug PASS, Release PASS e clean Release PASS, todos com testes. O único warning de configuração é uma depreciação no CMake da Raylib externa, não no código do jogo.

## Deferred / validation limits

- **DEFERRED — Playtest visual humano:** apps nativos não foram expostos pela ferramenta de automação; UI não foi validada por screenshots nesta máquina. Motivo: limitação concreta do ambiente, não falha conhecida do código.
- **DEFERRED — Sanitizers:** clang/gcc, clang-tidy e cppcheck não estão instalados. O modo `ENABLE_SANITIZERS` foi preparado para toolchains GCC/Clang, mas não foi executado.
- **DEFERRED — Fetch em rede limpa:** o clone novo do Raylib ficou preso na rede. O clean build recompilou tudo em `build-clean2` usando os sources locais das mesmas versões fixadas; o caminho de download em uma máquina sem cache não foi concluído nesta sessão.
- **ACCEPTED — Arte procedural:** existem 0 spritesheets por direção explícita do usuário; validação de PNG/metadata não se aplica.
- **ACCEPTED — Balanceamento:** nenhuma alteração numérica especulativa foi feita sem telemetria de runs humanas. Matriz funcional, soak e stress passaram.

## Final summary

- Critical issues found: 0; fixed: 0.
- High issues found: 3; fixed: 3.
- Medium issues found: 5; fixed: 5.
- Deferred release-blocking issues: 0.
- Build status: Debug PASS; Release PASS; Clean Release PASS.
- Release status: pacote portátil criado e smoke test com shutdown gracioso PASS.

## Hotfix pré-Etapa 7

- **FIXED — Pause por ESC:** a tecla é lida uma vez em `Application::Update`; a UI não processa mais `KEY_ESCAPE`. A transição encerra o update do frame e somente `Playing` executa `GameSession::Update`.
- **FIXED — limite retangular em janela ampla:** a cobertura procedural fixa foi substituída por cobertura baseada nas dimensões reais da janela mais margem. A linha rígida da arena virou fade de 64 px e a composição Playing/Paused ganhou fade discreto de 48 px antes de HUD/pause.
- **VALIDATION:** testes automatizados Debug, Release e Clean Release cobrem o mapeamento de estados e o gate que permite atualização da simulação exclusivamente em Playing. Teste visual/interativo de janela continua indisponível porque nenhuma superfície nativa foi exposta pela automação desta máquina.

## Etapa 7 — arquitetura multimodo

- `GameModeManager`, `IGameMode`, `SurvivalMode`, placeholder bloqueado de `ExpeditionMode`, `RunContext`, `RunOutcome` e `RunResult` foram adicionados.
- Wave progression, limite de timer, boss milestones, pressão de adds durante boss, chest intermediário e condição de vitória foram removidos da sessão/manager de boss e concentrados no Survival.
- Busca de dependências confirmou que Player e sistemas compartilhados não incluem nem consultam tipos de modo.
- Regressões automatizadas cobrem bloqueio do Expedition, start/reset/exit do Survival, waves, level-up, elites, bosses, chest, evolução, Game Over, restart, vitória e snapshot de resultado.
# Stage 8 audit scope

Regression coverage now includes all three event selections, blocked event starts,
all five special waves, shield/phasing/summoning, variants, both miniboss attack and
death paths, Phase Dash invulnerability/cooldown/reset, existing boss stress,
pool/grid stress, Escape state mapping and resized viewport fade calculations.
Expedition was unavailable at the time of the Stage 6 audit; Stage 11 now provides its playable MVP.
