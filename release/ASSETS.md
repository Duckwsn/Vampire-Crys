# Assets finais — versão 0.1.0

## Direção visual

A pedido do projeto, a apresentação final desta etapa é **100% geométrica e procedural**. Não existem PNGs, texturas, fontes externas ou spritesheets. Isso substitui deliberadamente a direção pixel-art do roteiro original: o protagonista é um quadrado luminoso de cor mutável e os demais elementos são composições originais de polígonos, linhas, anéis e partículas Raylib.

## Inventário

| Categoria | Implementação | Dimensão lógica | Animação | Origem/licença |
|---|---|---:|---|---|
| Player | quadrado em camadas, aura, outline e direção | raio gameplay 18 px | hue, aura, pulso, movimento, hit flash | código original |
| Enemies | cinco silhuetas compostas e elites com aura/emblema | raio da definição | oscilação, runas, carga e hit flash | código original |
| Bosses | Wyrm segmentado; Herald poligonal; Ascended energizado | raio gameplay | idle/move/cast/hit/phase | código original |
| Armas/evoluções | seis famílias de símbolos e projéteis | slots 36–48 px | rotação, pulso, trail e impacto | código original |
| XP/pickups/chest | cristais, cruz, magneto, núcleo e cofre | raio gameplay | pulse, rotação e brilho | código original |
| Environment | solo, pedras, plantas e rachaduras | células 64 px | estático por hash do mundo | código original |
| UI/VFX | painéis, ícones, sparks, quads, shards, rings e texto | variável | hover, lifetime, alpha e rotação | código original |
| Audio | 20 cues e 4 loops sintetizados em memória | 22.050 Hz mono float | pitch sutil e fades | código original |

## Spritesheet validation

Quantidade de spritesheets: **0**. Dimensões de imagem, frame size, grade, padding, margem, gutter e transparência PNG não se aplicam. Nenhum arquivo falso foi criado para simular validação. Se imagens forem introduzidas futuramente, metadata e validação matemática deverão ser adicionadas antes do carregamento.

`VisualStyle` é o ponto único de renderização. Não há carregamento de textura no gameplay. `AudioManager` cria todos os buffers no startup e os libera antes de fechar o dispositivo; arquivos ausentes não causam crash.
