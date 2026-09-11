# Final Audit

Data da auditoria: 2026-09-10. Versão alvo: 0.1.0, release interno.

## Critical

Nenhum crash, undefined behavior evidente ou fluxo principal quebrado foi encontrado no baseline Release.

## High

- **OPEN — Portabilidade do config:** `VAMPIRECRYS_CONFIG_DIR` embute o diretório absoluto do checkout; o binário distribuído pode ignorar seu próprio `config/waves.json`.
- **OPEN — Debug em Release:** atalhos de cheat e visualizações F2/F3 estão disponíveis sem uma definição de build específica.
- **OPEN — Build Debug MSVC:** compilação concorrente apresentou C1041 no PDB; `/FS` não está configurado.

## Medium

- **OPEN — Settings:** controles funcionam apenas durante a execução; não há leitura/gravação segura nem fallback para JSON corrompido.
- **OPEN — Config validation:** interval/count/cap têm clamps, mas start/end, run duration, caps especiais e pesos precisam validação completa.
- **OPEN — Stat safety:** multiplicadores acumulados não possuem clamps finais centralizados para cooldown, área, movimento, crítico e regeneração.
- **OPEN — Packaging:** não existe diretório portátil `release/` nem instrução curta junto ao executável.

## Low

- **OPEN — Overlay:** falta frame time explícito e identificação clara de build/seed.
- **OPEN — Documentation:** README ainda chama o projeto de MVP em partes e não documenta contrato de reset/prioridade final.

## Performance

Baseline Release passou os testes de 500 inimigos/250 projéteis/500 XP/1.000 partículas e cenário de boss. Hot paths usam pools e Spatial Grid; `PrepareUpgradeChoices` aloca apenas durante pausa de level-up, não no gameplay.

## Memory / Resource Management

Entidades são value-types em pools pré-dimensionados. Spawns sobrescrevem slots com `{}` antes de ativar, evitando vazamento de estado. Sons são carregados uma vez e descarregados antes de `CloseAudioDevice`. Não há `new/delete/malloc/free` manual no projeto.

## Gameplay

Testes existentes cobrem armas, passivas, elites, filas de level-up, milestones, bosses, chests e seis evoluções. A prioridade atual é `GameOver > Victory > LevelUp > ChestReward`, determinística no `Application`.

## UI

Estados MainMenu, Playing, Paused, Settings, LevelUp, ChestReward, GameOver e Victory existem. Janela mínima 960×540; auditoria visual automatizada não está disponível nesta máquina.

## Audio

Buffers procedurais têm owner único, throttling e transição musical. Falta persistência dos volumes.

## Assets

Direção intencionalmente procedural: 0 spritesheets e 0 imagens. Validação de grade/metadata não se aplica; não existem falsos assets.

## Documentation

README, arquitetura, decisões, balanceamento, performance e assets existem. Serão sincronizados com as correções finais.

## Build / Packaging

Baseline: Release PASS e testes PASS. Debug FAIL por C1041 de PDB em compilação concorrente. Warning observado é de depreciação no CMake da dependência Raylib vendorizada por FetchContent, não do código do jogo.
