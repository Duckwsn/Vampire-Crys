# Performance

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

## Stress de boss — Etapa 4

O teste dedicado combina Void Herald Ascended em fase 2, 100 inimigos comuns, 100 projéteis do jogador, padrões de projéteis do boss, 100 XP Orbs e 500 partículas. Na validação Release limpa final, 300 updates consumiram 7,59 ms totais (sem draw). No mesmo build, o Stress C geral consumiu 4,10 ms para 120 updates. Telegraphs, chains e chests usam armazenamento fixo/reutilizável; Tempest e Storm consultam a Spatial Grid e Phantom Barrage limita cada burst a 18 projéteis.
