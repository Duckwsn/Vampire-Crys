# Performance

## Expedition — Etapa 11

O pathing usa um flow field de quatro direções, reconstruído apenas quando o alvo muda
de célula. Walkability e slide são consultas constantes por entidade; os mapas não fazem
alocações por frame. Os encontros autorais atingem no máximo 33 spawns diretos, abaixo
do pool compartilhado e dos stress tests existentes.

Na validação Debug da Etapa 11, a suíte completa, incluindo catálogo, navegação,
Expedition e stress tests existentes, passou. Nesta máquina, a execução registrada foi:

- boss final + 100 inimigos + 100 tiros do jogador + tiros do boss + XP + 500 partículas:
  58,757 ms para 300 updates;
- 500 inimigos + 250 projéteis + 500 XP + 1.000 partículas: 135,240 ms para 120 updates.

São tempos totais do teste sintético, não estimativas de FPS.

## Meta atual

60 FPS em desktop moderno com 300–500 inimigos, 100+ projéteis de cada owner, 300–500 XP Orbs e efeitos básicos.

## Auditoria anterior à Etapa 3

- Auto-target percorria todos os 600 slots de inimigos por disparo.
- Cada projétil ativo testava todos os inimigos (`O(projectiles × enemies)`).
- Flame Ring e Guardian Orbs percorriam o pool inteiro por tick/orb.
- Separação verificava somente oito slots adjacentes: barata, mas espacialmente imprecisa.
- Pools já eram estáveis e pré-alocados; XP update era linear e sem busca adicional.

## Spatial Grid

A arena fixa de 4000×4000 usa células de 128 px. O tamanho cobre aproximadamente duas a quatro larguras de inimigos comuns e reduz candidatos sem gerar células excessivamente pequenas. São 32×32 células reutilizadas; cada inimigo é inserido apenas pela posição central, evitando duplicatas. `QueryCircle` calcula o retângulo de células e a colisão geométrica final permanece obrigatória.

A grid é reconstruída após movimento/spawn e atende target, projéteis, AoE, orbitais e separação. Ela armazena somente índices estáveis do pool, nunca pointers.

## Pools

- Enemies: 600
- Projectiles compartilhados: 800, com owner lógico
- XP Orbs: 800
- Pickups: 100
- Particles: 1600, com prioridade e substituição de cosméticos por efeitos importantes
- Area effects: 96
- Boss telegraphs/hazards: 48
- Treasure chests: 4

## Cenários de stress

Os testes automatizados incluem cenários A/B/C até 500 inimigos, 150 projéteis do jogador, 100 projéteis inimigos, 500 XP Orbs e 1000 partículas. No build Release local, a validação final do Stress C executou 120 updates em 18,09 ms totais (cerca de 0,15 ms/update, sem draw). A inicialização visual permanece limitada a 60 FPS por VSync; não foi feita medição comparável de renderização para prometer FPS em outro hardware.

## Gargalos restantes

Rebuild e queries ainda são single-threaded. XP Orbs continuam atualização linear, intencionalmente simples. Renderização não usa culling fino; essas áreas poderão ser medidas novamente após assets definitivos.

## Art pass — Etapa 5

O art pass não adiciona texturas, shaders por entidade ou carregamento durante ataques. O ambiente é limitado à região visível. Trails têm budget de 40 projéteis por frame, damage text agrega hits e partículas ficam limitadas a 1.600 slots. O stress final mede CPU sem draw/GPU e imprime seu próprio tempo; nenhuma promessa de FPS é inferida desse número.

No build Release final de 2026-09-10, Stress C executou 120 updates em 4,20 ms e o cenário de boss executou 300 updates em 6,13 ms. Antes do art pass, a última medição registrada era 4,10 ms e 7,59 ms, respectivamente. Como são execuções curtas sujeitas a ruído e sem draw, o resultado confirma ausência de regressão catastrófica, não um ganho garantido de performance.

## Auditoria final — Etapa 6

No clean Release build, Stress C (500 inimigos, 150 projéteis do jogador, 100 hostis, 500 XP e 1.000 partículas) executou 120 updates em **6,44 ms**. O cenário de boss final em fase 2 (100 inimigos, 100 tiros do jogador, projéteis do boss, 100 XP e 500 partículas) executou 300 updates em **5,32 ms**. São tempos totais de CPU da simulação e não incluem draw, VSync ou GPU.

O pool de inimigos tem cap intencional de 600; por isso um teste de 700 entidades não representa uma configuração suportada. Não foi medido FPS visual nesta máquina porque a captura/automação de apps nativos não estava disponível. Os gargalos aceitos permanecem update linear de XP e rebuild single-thread da grid; nenhum deles causou falha nos cenários medidos.

## Stress de boss — Etapa 4

O teste dedicado combina Void Herald Ascended em fase 2, 100 inimigos comuns, 100 projéteis do jogador, padrões de projéteis do boss, 100 XP Orbs e 500 partículas. Na validação Release limpa final, 300 updates consumiram 7,59 ms totais (sem draw). No mesmo build, o Stress C geral consumiu 4,10 ms para 120 updates. Telegraphs, chains e chests usam armazenamento fixo/reutilizável; Tempest e Storm consultam a Spatial Grid e Phantom Barrage limita cada burst a 18 projéteis.
# Stage 8 performance notes

All new enemies, summons, elites and minibosses reuse the fixed 600-enemy pool.
Special-wave groups are capped by both threat budget and the current wave cap;
Necromancer summons have a global cap of 24. Runtime event modifiers are scalar
queries, special compositions use fixed/local storage, and no new per-frame heap
allocation was introduced. Actual Stage 8 timing output is recorded by the systems
test executable during final validation rather than estimated here.

Final Release measurements on the delivery machine (single runs, so not a formal
benchmark): boss stress with final boss, 100 enemies, 100 player projectiles, boss
shots, XP and 500 particles completed 300 updates in 6.1454 ms; the pool/grid stress
with 500 enemies, 250 projectiles, 500 XP and 1000 particles completed 120 updates in
4.5872 ms. The final Release test process passed in 0.50 seconds via CTest; the
fresh Clean Release passed in 0.21 seconds (individual test time).

## Stage 9 implementation constraints

The new catalog does not change pool capacities. Soul Scythe performs one Spatial Grid query per sweep and stores only eight fixed visual arcs. Frost and Blood reuse the 800-projectile pool; Blood distributes targets with one query per volley, stores stable enemy-pool indices and caps volleys at 14. Gravity Well reuses the 96-area pool and queries the grid only on its 8–15 scheduled ticks. Slow adds two scalar fields to enemies/projectiles/bosses and allocates nothing per frame. Character portraits and weapon effects are immediate geometric draw calls with no texture memory.

Final Stage 9 Release validation on 2026-09-11 measured **7.9534 ms total / 300 updates** for the final-boss stress and **8.7304 ms total / 120 updates** for Stress C (500 enemies, 250 projectiles, 500 XP and 1,000 particles). These are single-run CPU simulation timings printed by `VampireCrysTests`, not FPS or GPU measurements. The final CTest runs passed in 0.59s on the regular Release tree and 0.94s after rebuilding the clean Release tree.

## Stage 10 implementation constraints and validation

`SurvivalRunConfig`, `RunModifiers` and score breakdown are fixed-size value types. Mutators do not allocate per frame. Endless reuses the existing enemy, projectile, XP, pickup, area, telegraph and particle pools; cycle scaling changes only bounded scalar inputs. The existing director cap remains active, boss encounters cull normal enemies to 30, one boss can be active, and the Endless scaling lookup clamps at cycle 8.

Clean Release validation on 2026-09-11 measured **8.8241 ms total / 300 updates** for the final-boss scenario with 100 enemies, 100 player shots, boss shots, XP and 500 particles, and **6.1176 ms total / 120 updates** for Stress C with 500 enemies, 250 projectiles, 500 XP and 1,000 particles. These are CPU simulation totals from one run of `VampireCrysTests`, not frame-time or GPU benchmarks. The complete clean Release CTest suite passed in **0.41 s**. Extreme Nightmare A10 with three mutators and Endless cycle 5/soft-cap inputs is also checked for finite, bounded modifiers.
# Verificações da Etapa 12

- O teste automatizado gera e valida 100 seeds de rota, além de conferir repetibilidade e variedade.
- Ataque manual reutiliza os pools existentes de projéteis/áreas e não executa busca automática
  quando nenhum botão está pressionado.
- O stress test existente continua cobrindo 500 inimigos, 250 projéteis, 500 orbs de XP e 1000
  partículas; o cenário de boss também cobre inimigos, disparos dos dois lados e partículas.
- Charge do Guardian usa a navegação já carregada e termina ao colidir com geometria inválida.

Tempos impressos pelos testes são diagnósticos da máquina, não metas nem FPS garantido.
