# Vampire Crys — Run completa

Survivors / Bullet Heaven 2D completo feito em C++17 com Raylib. A versão atual oferece Survival configurável com run padrão de 15 minutos ou Endless, quatro personagens jogáveis, dez armas e evoluções, bosses, identidade geométrica procedural, HUD, VFX e áudio sintetizado original.

## Requisitos

- CMake 3.20+
- Compilador com suporte a C++17
- Git e acesso à internet na primeira configuração, caso Raylib não esteja instalada

Raylib 5.5 e nlohmann/json 3.11.3 são obtidas automaticamente e fixadas por versão quando não há pacote local.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

No Windows com gerador Visual Studio, execute:

```powershell
.\build\bin\VampireCrys.exe
```

O CMake mantém o executável em `build/bin` tanto em geradores de configuração única quanto no Visual Studio.

Para executar os testes de sistemas e pools:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Para gerar a distribuição portátil após o build Release:

```powershell
cmake --install build --config Release --component Runtime --prefix release/game
```

Para usar uma instalação local de Raylib, ela deve fornecer um pacote CMake detectável por `find_package(raylib)`. Na ausência dele, a dependência é obtida automaticamente.

## Modos disponíveis

- **Survival — Available:** run de 15 minutos ou Endless, três dificuldades, Ascension 0–10, oito mutators, seis challenges, waves, elites, bosses, chests e evoluções.
- **Expedition — Available:** cinco arenas autorais lineares, encontros próprios, elite,
  recompensas entre fases e confronto final com o Void Herald. Build, HP, XP, nível,
  estatísticas e seed persistem entre fases; Restart recomeça a Fase 1 limpa.

Fluxo do menu: `PLAY → SELECT MODE → SELECT SURVIVOR`. Survival segue para seu setup;
Expedition inicia diretamente na primeira fase. Quick Start preserva Normal/A0/Standard;
Start usa a configuração escolhida. `Esc` volta uma tela durante as seleções.

## Personagens jogáveis

| Personagem | Arma inicial | Identidade |
|---|---|---|
| Hunter | Arc Bolt | baseline versátil; +15% contra inimigos acima de 80% HP |
| Pyromancer | Flame Ring | -10% HP; +10% área/duração; área temporária após hits AoE |
| Sentinel | Guardian Orbs | +20% HP, +2 armor, -8% movimento; próximo hit reduzido após 5s ileso |
| Occultist | Void Lance | -12% HP; +0,35 Luck e +4% crítico |

Todos usam a mesma implementação de `Player`, pool de upgrades e seis slots de arma/passiva. A seleção vale para a run atual e é preservada em Restart.

## Arsenal

As dez armas automáticas são Arc Bolt, Flame Ring, Guardian Orbs, Spectral Fan, Void Lance, Thunder Cannon, Soul Scythe, Frost Shards, Blood Needles e Gravity Well. As quatro últimas acrescentam sweep direcional, Slow, homing distribuído e controle de área por ticks. Cada arma chega ao nível 8; passivas chegam ao nível 5; evoluções continuam exclusivas de chests.

## Controles

- `WASD` ou setas: mover
- `Espaço`: Phase Dash
- `Esc`: pausar/retomar

### Survival

- O arsenal de até 6 armas ataca automaticamente; o mouse não é necessário para o combate.

### Expedition

- Mouse: mirar no mundo
- Botão esquerdo: arma Primary
- Botão direito: arma Secondary
- `Q`: trocar Primary e Secondary sem reiniciar cooldowns
- Limite: exatamente 2 armas ativas; novas armas com slots cheios abrem confirmação de substituição
- Mouse ou `1`, `2`, `3`: escolher upgrade ou recompensa de chest
- `Enter`: iniciar/reiniciar
- `Esc`: pausar/retomar; voltar em Settings
- `F2`: hitboxes de colisão (somente build Debug)
- `F3`: debug overlay (somente build Debug)
- `F4`: concede 100 XP (debug)
- `F5`: cura completamente (debug)
- `F6`: alterna invulnerabilidade (debug)
- `F7`: adiciona 100 inimigos (debug)
- `F8`: adiciona 300 inimigos (debug)
- `F9`: elimina todos os inimigos ativos (debug)
- `F10`: avança 60 segundos da run (debug)
- `F11`: invoca Flame Wyrm (debug)
- `F12`: invoca Void Herald (debug)
- `B`: invoca Void Herald Ascended (debug)
- `K`: remove 10% do HP máximo do boss ativo (debug)
- `E`: prepara Arc Bolt Lv8 + Focus Crystal e cria um chest de teste (debug)
- `G`: concede/melhora ciclicamente uma das quatro armas da Etapa 9 (debug)
- `H`: prepara a evolução da última arma concedida e cria um chest (debug)
- `N/C`: inicia/encerra evento; `M`: special wave; `V/X`: minibosses (debug)
- No setup com F3 ativo: `D/A/L/U/J` alternam dificuldade, Ascension, Endless, mutator e challenge (debug)
- `Y`: avança cinco minutos da run para validar ciclos Endless (debug)
- Expedition com F3: `I` conclui o encontro, `O` abre a recompensa e `P` vai à Fase 5 (debug)

Os atalhos F2–F12/B/K/E são compilados para entrada apenas na configuração Debug. No Release, cheats e visualizações permanecem indisponíveis.

## Visual, resolução e configurações

A janela é redimensionável com mínimo de 960×540; HUD, menus e câmera usam as dimensões atuais. O terreno procedural cobre dinamicamente o viewport e recebe uma transição discreta de 48 px nas bordas em Playing/Paused, sem alterar câmera, proporção ou coordenadas do mouse. Settings controla Master, Music e SFX de 0–100%, screen shake, fullscreen e idioma English/PT-BR em tempo real. Os valores são gravados ao sair de Settings e no encerramento, com substituição protegida em `settings.json`; arquivo ausente ou corrompido usa defaults seguros.

Não há spritesheets: todo o catálogo é desenhado por `VisualStyle`. O ambiente usa hash das coordenadas, portanto o mesmo lugar conserva as mesmas variações. Veja [ASSETS.md](ASSETS.md).

## Áudio

`AudioManager` sintetiza no startup 20 efeitos e quatro loops (menu, gameplay, boss e vitória), todos originais e sem arquivos externos. Cues repetitivos têm cooldown e pitch discreto; músicas fazem fade. Os buffers Raylib são descarregados no shutdown.

## Configuração e release

As waves ficam em `config/waves.json`, copiado automaticamente para o diretório do executável. O loader resolve o caminho a partir da aplicação, valida ranges, caps e pesos e usa defaults internos quando necessário. O pacote `release/` contém EXE, `config/`, `assets/` e documentação; Raylib é ligada estaticamente no build Windows.

Todas as armas atacam automaticamente. Cada personagem começa somente com sua arma inicial no nível 1.

Bosses intermediários sempre deixam um chest. Uma arma evolui somente no chest quando está no nível 8 e sua passiva exigida pertence ao loadout; a passiva não precisa estar maximizada. Se várias evoluções estiverem elegíveis, o chest oferece até três para escolha. Sem evolução disponível, oferece upgrades úteis ou restauração completa.

Consulte [BALANCING.md](BALANCING.md) para progressões, fórmulas, inimigos e raridades.
Os rulesets, score e agenda Endless estão detalhados em [SURVIVAL_ENDGAME.md](SURVIVAL_ENDGAME.md).
# Stage 8 - Survival Expansion I

Survival now includes three run events (Blood Moon, The Swarm and Elite Hunt), five
threat-budget special waves, three scheduled miniboss encounters and the Phase Dash
active ability. Expedition uses the same shared combat systems through its own stage director.

Press `SPACE` while playing to Phase Dash in the current movement direction (or the
last facing direction). It lasts 0.15 seconds, grants a brief invulnerability window
and has an 8-second cooldown shown in the HUD. Pause freezes the ability and every
encounter timer because only the Playing state advances simulation.

The F3 debug overlay now includes the run seed, active event, special wave, miniboss,
threat budget and variant count. Debug-only controls: `N` requests the next event,
`C` ends it, `M` cycles special waves, `V` spawns Grave Warden and `X` spawns Void
Stalker. Existing boss, XP and stress shortcuts remain available.
