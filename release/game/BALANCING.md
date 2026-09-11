# Balanceamento final — versão 0.1.0

## Expedition — Etapa 11

O orçamento autoral cresce em cinco estágios: 23 inimigos na Fase 1, 33 na Fase 2,
13 na Fase 3 (incluindo Grave Warden elite), 25 na Fase 4 (incluindo Void Stalker elite
e summons limitados) e um Void Herald na Fase 5. HP cresce 16% por estágio; dano parte
de 88% e cresce 8% por estágio. XP comum é multiplicado por 2,1.

Cada fase não final oferece três rewards filtrados. A elite prefere raridades acima de
Common. A conclusão cura 12% do HP faltante; Field Mending cura 30% do HP máximo e
Ancient Insight concede 150 XP. O Void Herald usa 72% do HP e 88% do dano-base.

Todos os valores abaixo são bases antes dos modificadores globais. Dano final é `base × damageMultiplier`; cooldown final é `base × cooldownMultiplier` (mínimo global 0,35); áreas, velocidade e duração usam seus multiplicadores correspondentes.

## Weapons

| Weapon | Level | Damage | Cooldown | Projectiles / Ticks | Area | Other Effects |
|---|---:|---:|---:|---:|---:|---|
| Arc Bolt | 1 | 13 | 0,72s | 1 | raio 6 | speed 560 |
| Arc Bolt | 2 | 16,25 | 0,72s | 1 | raio 6 | — |
| Arc Bolt | 3 | 16,25 | 0,634s | 1 | raio 6 | — |
| Arc Bolt | 4 | 16,25 | 0,634s | 2 | raio 6 | leque 10° |
| Arc Bolt | 5 | 16,25 | 0,634s | 2 | raio 6 | speed 700 |
| Arc Bolt | 6 | 16,25 | 0,634s | 2 | raio 6 | piercing 1 |
| Arc Bolt | 7 | 21,13 | 0,634s | 2 | raio 6 | piercing 1 |
| Arc Bolt | 8 | 21,13 | 0,634s | 3 | raio 6 | piercing 2 |
| Flame Ring | 1 | 10 | 3,40s | 2 ticks | raio 105 | duração 0,55s |
| Flame Ring | 2 | 12 | 3,40s | 2 ticks | raio 105 | — |
| Flame Ring | 3 | 12 | 3,40s | 2 ticks | raio 120,75 | — |
| Flame Ring | 4 | 12 | 3,06s | 2 ticks | raio 120,75 | — |
| Flame Ring | 5 | 12 | 3,06s | 3 ticks | raio 120,75 | — |
| Flame Ring | 6 | 12 | 3,06s | 3 ticks | raio 144,9 | — |
| Flame Ring | 7 | 15,6 | 3,06s | 3 ticks | raio 144,9 | — |
| Flame Ring | 8 | 15,6 | 2,601s | 4 ticks | raio 144,9 | duração 0,80s |
| Guardian Orbs | 1 | 8 | contato | 1 orb | raio 13 | órbita 94, speed 1,65 |
| Guardian Orbs | 2 | 10,4 | contato | 1 orb | raio 13 | — |
| Guardian Orbs | 3 | 10,4 | contato | 2 orbs | raio 13 | — |
| Guardian Orbs | 4 | 10,4 | contato | 2 orbs | raio 13 | orbit speed 2,145 |
| Guardian Orbs | 5 | 10,4 | contato | 2 orbs | raio 16,9 | — |
| Guardian Orbs | 6 | 10,4 | contato | 3 orbs | raio 16,9 | — |
| Guardian Orbs | 7 | 14,04 | contato | 3 orbs | raio 16,9 | — |
| Guardian Orbs | 8 | 14,04 | contato | 4 orbs | raio 16,9 | órbita 112 |
| Spectral Fan | 1 | 8 | 1,65s | 3 | raio 5 | spread 42° |
| Spectral Fan | 2 | 10 | 1,65s | 3 | raio 5 | — |
| Spectral Fan | 3 | 10 | 1,65s | 4 | raio 5 | — |
| Spectral Fan | 4 | 10 | 1,65s | 4 | raio 5 | spread 52° |
| Spectral Fan | 5 | 10 | 1,65s | 5 | raio 5 | — |
| Spectral Fan | 6 | 10 | 1,65s | 5 | raio 5 | piercing 1 |
| Spectral Fan | 7 | 10 | 1,403s | 5 | raio 5 | piercing 1 |
| Spectral Fan | 8 | 10 | 1,403s | 7 | raio 5 | piercing 1 |
| Void Lance | 1 | 28 | 2,25s | 1 | raio 9 | speed 760, piercing 4 |
| Void Lance | 2 | 36,4 | 2,25s | 1 | raio 9 | piercing 4 |
| Void Lance | 3 | 36,4 | 2,25s | 1 | raio 9 | piercing 5 |
| Void Lance | 4 | 36,4 | 2,25s | 1 | raio 12,15 | piercing 5 |
| Void Lance | 5 | 36,4 | 2,25s | 1 | raio 12,15 | speed 950 |
| Void Lance | 6 | 36,4 | 2,25s | 1 | raio 12,15 | piercing 7 |
| Void Lance | 7 | 36,4 | 1,845s | 1 | raio 12,15 | piercing 7 |
| Void Lance | 8 | 49,14 | 1,845s | 1 | raio 15,19 | piercing 7 |
| Thunder Cannon | 1 | 42 | 4,20s | 1 | explosão 82 | speed 260 |
| Thunder Cannon | 2 | 52,5 | 4,20s | 1 | explosão 82 | — |
| Thunder Cannon | 3 | 52,5 | 4,20s | 1 | explosão 98,4 | — |
| Thunder Cannon | 4 | 52,5 | 3,696s | 1 | explosão 98,4 | — |
| Thunder Cannon | 5 | 52,5 | 3,696s | 1 | explosão 98,4 | speed 325 |
| Thunder Cannon | 6 | 68,25 | 3,696s | 1 | explosão 98,4 | — |
| Thunder Cannon | 7 | 68,25 | 3,696s | 1 | explosão 123 | — |
| Thunder Cannon | 8 | 68,25 | 3,696s | 2 | explosão 123 | spread 12° |

Guardian Orbs usa cooldown de 0,4s por alvo. Flame Ring faz um único roll crítico por ativação e aplica o resultado nos ticks, evitando RNG por frame.

## Passives

| Passive | Level | Efeito total acumulado |
|---|---:|---|
| Swift Boots | 1–5 | +5%, +10%, +15%, +20%, +25% da velocidade-base |
| Power Core | 1–5 | +10%, +20%, +30%, +40%, +50% de dano global |
| Chrono Gear | 1–5 | cooldown ×0,93; ×0,865; ×0,804; ×0,748; ×0,696 |
| Expansion Rune | 1–5 | área ×1,10; ×1,20; ×1,30; ×1,40; ×1,50 |
| Magnet Stone | 1–5 | magnet 135; 165; 195; 225; 255 |
| Vital Heart | 1–5 | +15/+0,2; +30/+0,4; +45/+0,6; +60/+0,8; +75 HP/+1,0 HP/s |
| Iron Shell | 1–5 | 1; 2; 3; 4; 5 armor |
| Lucky Charm | 1–5 | +0,2…+1,0 Luck e crítico-base de 6%…10% |
| Focus Crystal | 1–5 | +10%…+50% projectile speed e +1%…+5% crítico |
| Ember Core | 1–5 | +6%…+30% área e duração |
| Orbital Engine | 1–5 | +5%…+25% área e +8%…+40% duração |
| Wind Sigil | 1–5 | +3%…+15% movimento e +6%…+30% projectile speed |
| Piercing Eye | 1–5 | +1,5%…+7,5% crítico e +1…+5 piercing global |
| Titan Core | 1–5 | +5%…+25% dano global e +8…+40 max HP |

Armadura usa `max(1, incomingDamage - armor)`. Cada nível de Vital Heart cura a mesma quantidade adicionada ao HP máximo.

## Rarity e Luck

Pesos-base: Common 60, Uncommon 25, Rare 12, Epic 3. Para Luck entre 0 e 3:

- Common: `max(20, 60 - 12 × Luck)`
- Uncommon: `25 + 2 × Luck`
- Rare: `12 + 6 × Luck`
- Epic: `3 + 4 × Luck`

## Enemies

| Enemy | HP | Speed | Damage | XP | Special behavior |
|---|---:|---:|---:|---:|---|
| Ghoul | 32 | 66 | 9 contato | 4 | perseguidor básico |
| Swarmer | 14 | 118 | 6 contato | 3 | pequeno, rápido, nasce em grupos de 3 |
| Brute | 105 | 39 | 18 contato | 12 | grande, lento, 65% resistência a knockback |
| Cultist | 42 | 54 | 8 contato / 10 projétil | 9 | mantém ~275 de distância; tiro a cada 2,2s |
| Bomber | 48 | 62 | 8 contato / 24 explosão | 10 | telegraph 0,9s; raio 118 |

Se o Bomber morrer antes do ataque, explode com 50% do dano (12) e raio reduzido (72). A progressão de HP, dano, velocidade, XP e composição foi substituída na Etapa 3 pelas curvas e fases do `WaveDirector` descritas abaixo e configuradas em `config/waves.json`.

## Run de 15 minutos

| Fase | Tempo | Intervalo | Quantidade | Cap | Composição principal |
|---|---|---:|---:|---:|---|
| Early I | 0:00–1:00 | 0,90s | 2 | 90 | Ghoul |
| Early II | 1:00–2:00 | 0,76s | 2 | 130 | Ghoul/Swarmer |
| Build Up | 2:00–3:00 | 0,64s | 3 | 180 | +Brute |
| Mixed | 3:00–4:30 | 0,54s | 3 | 230 | +Cultist |
| Pressure | 4:30–7:00 | 0,45s | 4 | 300 | +Bomber |
| Horde | 7:00–9:30 | 0,36s | 5 | 360 | mais Swarmer |
| High Pressure | 9:30–12:00 | 0,29s | 6 | 420 | mistura completa |
| Final Build | 12:00–14:00 | 0,23s | 7 | 470 | alta densidade |
| Final Horde | 14:00–15:00 | 0,18s | 8 | 500 | pressão final |

Os pesos exatos e caps por tipo estão em `config/waves.json`.

## Difficulty scaling

| Tempo | HP | Dano | Velocidade | XP |
|---|---:|---:|---:|---:|
| 0:00 | 1,00× | 1,00× | 1,00× | 1,00× |
| 5:00 | 1,25× | 1,10× | 1,05× | 1,20× |
| 10:00 | 1,63× | 1,28× | 1,10× | 1,40× |
| 15:00 | 2,00× | 1,45× | 1,15× | 1,60× |

Elites começam em 3:00 e variam de 1% a 8%. Recebem HP ×3, dano ×1,5, XP ×3, escala ×1,25 e velocidade ×1,04.

## Drops

| Origem | Health | Magnet | Bomb |
|---|---:|---:|---:|
| Normal | 0,50% | 0,25% | 0,10% |
| Elite | 4,00% | 2,00% | 1,00% |

Luck multiplica chances por `min(2, 1 + Luck × 0,35)`. Health recupera 20% do HP máximo, Magnet atrai todos os XP Orbs a 3× velocidade e Bomb causa 120 de dano em raio 850; elites não possuem exceção de morte automática.

## XP e nível esperado

A curva permanece `12 + (level - 1) × 8`. Rewards recebem scaling até 1,6× e elites entregam 3× XP. O alvo de balanceamento para uma run completa, com coleta consistente, é aproximadamente nível 35–55; bosses reduzem adds para preservar legibilidade.

## BOSSES

| Boss | Spawn | HP | Speed | Contact | Attacks | Telegraph | Phase 2 | Recovery |
|---|---:|---:|---:|---:|---|---:|---:|---:|
| Flame Wyrm | 05:00 | 4.200 | 72 | 14 | Breath 15/18; Pools 10/12 | 1,0–1,05s | 50% HP | 1,25s; ×0,82 P2 |
| Void Herald | 10:00 | 8.200 | 62 | 16 | Barrage 12; Beams 16 | 0,95s | 50% HP | 1,25s; ×0,82 P2 |
| Void Herald Ascended | 15:00 | 15.500 | 78 | 20 | Barrage 16; Beams 20 | 0,72s | 40% HP | 0,85s; ×0,82 P2 |

Fire Breath usa range 390 e cone 58°/66°, duração ativa 0,65s. Flame Pools usam 4/5 círculos de raio 64 por 4,0/4,8s e ticks de 0,65s. Radial Barrage lança 12/16 projéteis no Herald; fase 2 adiciona anel rotacionado. Ascended lança 20×2. Void Beams usam 2/3 linhas no Herald e 3/4 no Ascended, width 42 e range 1.050. Durante qualquer boss há no máximo 30 adds e spawn interval ×5.

## EVOLUTIONS

| Base Weapon | Required Passive | Evolution | Behavior | Estimated Power Increase |
|---|---|---|---|---:|
| Arc Bolt Lv8 | Focus Crystal Lv1+ | Storm Arc | 38 damage, até 5 alvos distintos, saltos 285 | ~2,0× em grupos |
| Flame Ring Lv8 | Ember Core Lv1+ | Inferno Halo | halo 168, damage 17, tick 0,25s | ~2,2× sustentado |
| Guardian Orbs Lv8 | Orbital Engine Lv1+ | Celestial Guard | 6 orbs, 1,65× dano, burst radial de 10 | ~2,0× |
| Spectral Fan Lv8 | Wind Sigil Lv1+ | Phantom Barrage | 18 projéteis em 360°, cooldown 1,15s | ~2,3× cobertura |
| Void Lance Lv8 | Piercing Eye Lv1+ | Abyss Spear | damage 72, raio 18, piercing 1.000, speed 930 | ~1,8× |
| Thunder Cannon Lv8 | Titan Core Lv1+ | Tempest Cannon | primary 1,65× + 3 explosões de 52% | ~2,1× em área |

Passivas exigidas precisam apenas estar no loadout. Evoluções não possuem níveis adicionais e só são concedidas por chest.

## Limites defensivos de runtime

Valores normais ficam abaixo destes limites. A sanitização protege dados inválidos: move speed 80–600, cooldown multiplier 0,20–3, area/projectile speed/duration 0,25–5, critical chance 0–95%, Luck 0–3, armor 0–50, regen 0–20 e magnet radius 30–1.200. Dano recebido continua com mínimo 1 após armor.
# Stage 8 encounter values

- Blood Moon: 3-second warning, 34 seconds active, faster spawning, +8% enemy movement
  and a 1.5x relative elite roll.
- The Swarm: 3-second warning, 27 seconds active, 82% Swarmer normal composition and
  faster spawn cadence.
- Elite Hunt: 3-second warning, 29 seconds active, three marked elites, normal spawn
  pressure reduced to 72%; marked kills grant boosted XP and a useful pickup.
- Special waves last 12 seconds and spend at most a 30-point group budget. Costs range
  from 1 (Swarmer) to 8 (Necromancer), with the normal wave cap still enforced.
- Shielded Acolyte shield equals 42% max HP, absorbs 68% of incoming hits and recovers
  after 5 seconds when broken. Wraith phases for 1.15 seconds and has a 0.45-second
  harmless solidification window. Necromancers keep at most 3 personal summons and
  the run has a 24-summon global cap.
- Grave Warden alternates a 145-radius heavy slam and 225-radius shockwave. Void
  Stalker alternates a 250-unit telegraphed dash and five-projectile fan.

## Stage 9 characters

| Character | Max HP | Move | Other base differences | Trait |
|---|---:|---:|---|---|
| Hunter | 100 | 235 | baseline | +15% player damage to normal targets above 80% HP |
| Pyromancer | 90 | 235 | area/duration ×1.10 | each 5 AoE hits grants area ×1.12 for 3s |
| Sentinel | 120 | 216.2 | +2 armor | after 5s unharmed, next incoming hit ×0.65 |
| Occultist | 88 | 239.7 | +0.35 Luck, +4% crit | tradeoff is fully applied at reset |

## Stage 9 weapon levels

All modifiers are cumulative from the listed base.

| Weapon | Lv1 | Lv2–4 | Lv5–7 | Lv8 |
|---|---|---|---|---|
| Soul Scythe | 38 dmg, 1.55s, radius 132, arc 105° | +22% dmg, +15% radius, arc 130° | -16% CD, +28% dmg, +18% radius | opposite second sweep |
| Frost Shards | 9 dmg, 3 shards, 1.28s, speed 500 | +22% dmg, +1 shard, -12% CD | +18% speed, +1 shard, +30% dmg | +1 shard, +1 pierce |
| Blood Needles | 5.5 dmg, 2 needles, 0.46s, speed 540 | +20% dmg, +1 needle, -16% CD | +18% speed, +1 needle, +28% dmg | +1 needle, -18% CD |
| Gravity Well | 11/tick, 4.8s CD, radius 145, duration 3s, 8 ticks | +20% dmg, +15% radius, +20% duration | -15% CD, +30% dmg, +20% radius | +3 ticks, +15% duration |

## Stage 9 passives

| Passive | Per level | Lv5 total / trigger |
|---|---|---|
| Executioner Sigil | +6% damage below 30% enemy HP | +30% (half effectiveness on minibosses/bosses) |
| Frozen Heart | +8% Slow effectiveness, +0.2 armor | +40%, +1 armor |
| Soul Harvest | +4% XP from enemy deaths | +20% XP |
| Blood Pact | +8% global damage, -4 max HP | +40% damage, -20 HP; floor 20 |
| Gravitic Core | +6% duration, +3% area, +10% control | +30%, +15%, +50% |
| Soul Chain | +1 projectile at passive levels 2 and 4 | +2 projectiles, global cap applies |

## Stage 9 evolutions and status

| Recipe | Result | Final behavior |
|---|---|---|
| Soul Scythe Lv8 + Executioner Sigil | Reaper's Covenant | dual 1.32× radius sweeps, ×1.55 damage, every third attack is full circle |
| Frost Shards Lv8 + Frozen Heart | Absolute Zero | +2 shards, ×1.42 damage, stronger Slow; every second volley creates radius-215 nova |
| Blood Needles Lv8 + Blood Pact | Crimson Swarm | +5 needles, ×1.35 damage, faster CD and periodic +3 burst, capped at 14 |
| Gravity Well Lv8 + Gravitic Core | Singularity | ×1.35 radius, ×1.45 damage, +25% duration, +4 ticks, pull 155 |

Frost Slow is 0.74 movement for 1.6s; Absolute Zero shards use 0.60/2.4s and nova 0.58/2.8s. Reapplication keeps the strongest current multiplier and longest duration instead of multiplying stacks. Normal enemy movement never falls below 0.55. Minibosses receive 45% strength and 55% duration; bosses never fall below 0.88 and receive 40% duration. Bosses cannot be pulled.

## Stage 10 endgame rules

Difficulty uses three centralized tiers. Hard applies enemy HP 1.20, damage 1.15, spawn interval 0.90, elite chance 1.30, miniboss HP 1.18, boss HP 1.22 and boss damage 1.12. Nightmare uses 1.42, 1.30, 0.82, 1.70, 1.38, 1.48 and 1.25 respectively, plus pressure 1.10 and recovery chance 0.80. Normal is exactly 1.00.

Ascension 0–10 adds per level: enemy HP 2.5%, damage 1.8%, elite chance 6%, miniboss HP 3.5%, boss HP 3% and score 5%; spawn interval falls 1.2% per level with a floor. A4 accelerates events to 0.88, A6 adds pressure 1.08 and A8 accelerates special waves to 0.84. Complete mutator and challenge numbers are in `SURVIVAL_ENDGAME.md`.

Endless cycles are 15 minutes. Beyond cycle one, each capped cycle step adds enemy HP 22%, damage 14%, elite chance 22% and boss HP 30%, and removes 5% spawn interval. Cycle scaling stops growing after cycle 8; spawn interval floors at 0.32, speed caps at 1.25, pressure at 1.80, recovery chance floors at 0.10 and score multiplier caps at 8.00.

Score base: eligible kills ×10 + elites ×75 + minibosses ×300 + bosses ×1200 + whole seconds ×2. Difficulty is ×1.00/1.25/1.60, Ascension is ×(1 + 0.05A), followed by active mutator/challenge multipliers.
# Expedition dual-weapon balance — Etapa 12

- HP inimigo por profundidade: `1 + 0,16 * depth`; dano: `0,88 + 0,08 * depth`.
- Boss Expedition: 0,72x HP e 0,88x dano do perfil compartilhado.
- Elite squad usa cerca de 1,86x HP-base após os multiplicadores existentes, em vez de depender de
  3x adicional sobre todo o scaling de Expedition; oferece recompensa melhorada.
- Primeiro reward de Expedition garante uma opção de segunda arma. Ataques primários comuns têm
  cooldowns curtos; AoEs e controle permanecem secundários mais situacionais.
- Grave Warden: 1500 HP-base, velocidade 48, carga 520; Void Stalker: 1350 HP-base, velocidade 76.
