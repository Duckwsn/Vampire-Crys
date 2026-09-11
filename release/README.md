# Vampire Crys — Run completa

Survivors / Bullet Heaven 2D completo feito em C++17 com Raylib. A versão interna 0.1.0 oferece uma run de 15 minutos, seis armas e evoluções, bosses, identidade geométrica procedural, HUD, VFX e áudio sintetizado original.

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
cmake --install build --config Release --prefix release
```

Para usar uma instalação local de Raylib, ela deve fornecer um pacote CMake detectável por `find_package(raylib)`. Na ausência dele, a dependência é obtida automaticamente.

## Controles

- `WASD` ou setas: mover
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

Os atalhos F2–F12/B/K/E são compilados para entrada apenas na configuração Debug. No Release, cheats e visualizações permanecem indisponíveis.

## Visual, resolução e configurações

A janela é redimensionável com mínimo de 960×540; HUD, menus e câmera usam as dimensões atuais. Settings controla Master, Music e SFX de 0–100%, screen shake e fullscreen em tempo real. Os valores são gravados com substituição protegida em `settings.json`, ao lado do executável; arquivo ausente ou corrompido usa defaults seguros.

Não há spritesheets: todo o catálogo é desenhado por `VisualStyle`. O ambiente usa hash das coordenadas, portanto o mesmo lugar conserva as mesmas variações. Veja [ASSETS.md](ASSETS.md).

## Áudio

`AudioManager` sintetiza no startup 20 efeitos e quatro loops (menu, gameplay, boss e vitória), todos originais e sem arquivos externos. Cues repetitivos têm cooldown e pitch discreto; músicas fazem fade. Os buffers Raylib são descarregados no shutdown.

## Configuração e release

As waves ficam em `config/waves.json`, copiado automaticamente para o diretório do executável. O loader resolve o caminho a partir da aplicação, valida ranges, caps e pesos e usa defaults internos quando necessário. O pacote `release/` contém EXE, `config/`, `assets/` e documentação; Raylib é ligada estaticamente no build Windows.

Todas as armas atacam automaticamente. O jogador começa somente com Arc Bolt Lv1.

Bosses intermediários sempre deixam um chest. Uma arma evolui somente no chest quando está no nível 8 e sua passiva exigida pertence ao loadout; a passiva não precisa estar maximizada. Se várias evoluções estiverem elegíveis, o chest oferece até três para escolha. Sem evolução disponível, oferece upgrades úteis ou restauração completa.

Consulte [BALANCING.md](BALANCING.md) para progressões, fórmulas, inimigos e raridades.
