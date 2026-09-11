# Expedition MVP

## Fluxo jogável

`Mode Selection → Character Selection → Encounter → Reward → Route → ... → Boss → Victory`

Uma run percorre sete camadas de rota e escolhe arenas de um catálogo de layouts
irregulares desenhados manualmente. Cada mapa declara entrada, saída, zonas de spawn,
chão, vazio e obstáculos; todos são validados quanto à conectividade nos testes.

Expedition stages use hand-authored, irregular, fractured top-down combat spaces with
non-walkable negative space and intentionally composed footprints. The visual/layout
philosophy may take inspiration from the principles of authored action-roguelike arenas,
but all layouts and assets are original.

Combat, Elite e Reward formam as escolhas intermediárias. A última camada contém o
Void Herald e encerra a run quando ele morre. Eventos narrativos, Ascension e desafios
continuam fora do Expedition nesta etapa.

## Ciclo de encontro e reward

Cada encontro passa por `Intro → Combat → Reward → Transition → Route`. Grupos aparecem
em tempos e zonas declarados pelo catálogo, sem `WaveDirector`. Depois da última morte,
level-ups pendentes têm prioridade; em seguida surgem três rewards válidos. A escolha
a rota para a próxima decisão. A fase final usa `Boss death → Victory`.

O catálogo mantém itens máximos válidos, inclui novas aquisições e melhorias,
expõe evolução elegível, cura parcial e XP. A elite prefere opções não comuns. Entre
estágios também é curado 12% do HP que estiver faltando.

## Arquitetura de mapa e navegação

As máscaras ASCII separam layout e encontro. `#`, `E`, `X` e `S` são navegáveis; `.`
forma o vazio e `O` é prop sólido. O carregador extrai bounds, entrada, saída e spawn
zones. `WorldNavigation` oferece colisão/slide, posição segura, clamp da câmera e flow
field compartilhado para inimigos terrestres.

## Persistência e reset

Entre fases persistem personagem, PlayerStats, HP/XP/nível, armas, passivas, evoluções,
timer, contadores e seed. Entidades transitórias são limpas. Restart recria toda a run
na Fase 1 com o personagem e seed originais, sem manter upgrades ou inimigos.

## Debug

Em build Debug, `F3` também desenha a ocupação do mapa. `I` conclui o encontro atual,
`O` força a recompensa da fase e `P` carrega diretamente a fase do boss.

## Roadmap

A Etapa 12 poderá escolher o próximo ID do catálogo por meio de uma rota. Este MVP não
contém branches, nós, lojas, atos, salas procedurais nem eventos de Expedition.
# Etapa 12 — combate manual e rotas

Expedition usa mira manual independente do movimento. A aplicação converte o cursor de tela para
coordenadas de mundo pela `Camera2D`, limita alvos ao espaço caminhável e envia um `AttackRequest`
ao `WeaponManager`. LMB aciona o slot Primary, RMB o Secondary e `Q` troca as instâncias; portanto,
o cooldown permanece ligado à arma. O modo limita o loadout a duas armas, preserva os seis slots de
passivas e abre uma tela confirmável/cancelável quando uma arma nova exige substituição. Armas
evoluídas podem ser removidas, com aviso explícito de perda da evolução.

A rota é um DAG persistente de sete camadas gerado uma vez pela seed da run. O primeiro nó e o boss
são únicos; as cinco camadas intermediárias oferecem 2–3 alternativas conectadas. Há nós Combat,
Elite, Reward e Boss. Duas camadas obrigatoriamente Combat, junto do encontro inicial, garantem ao
menos três combates; escolhas Elite e Reward são sempre oferecidas e o boss é sempre alcançável.
Nós Reward não têm combate. Estados `Unavailable`, `Available`, `Current` e `Completed`, conexões e
caminho escolhido são desenhados no mapa. As arenas físicas continuam manuais, irregulares e
fraturadas; somente o grafo e a associação entre encontro e arena variam proceduralmente.

Grave Warden agora persegue a 48 unidades/s e alterna Charge (0,70 s de aviso, 0,48 s de avanço a
520 unidades/s, interrompido por colisão) com Heavy Slam (0,90 s de aviso, área de 150 e seis
projéteis radiais), separados por recuperação e cooldown. Void Stalker passou a 76 unidades/s,
telegraph de 0,68 s e downtime de 2,6 s. Nós Elite alternam deterministicamente entre minibosses e
um esquadrão elite, sem revelar qual encontro antes da entrada.
