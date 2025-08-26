# Estratégia de Navegação e Busca da Saída

Este documento descreve a estratégia atualmente implementada para encontrar a saída (objetivo) no labirinto, tanto no núcleo (`src/core/`) quanto no simulador (`simulator/`).

Principais componentes relacionados:
- `src/core/Navigator.hpp` e `src/core/Navigator.cpp`: decisão de movimento, observação de paredes, priorização de ações, uso de plano.
- `src/core/Planner.hpp`: planejamento de rota (BFS) sobre o `MazeMap`.
- `src/core/MazeMap.hpp`: representação do mapa conhecido (paredes N/E/S/W por célula).
- `src/core/Learning.hpp`: heurísticas e função `update_heuristic()` para reforço simples.

## Visão Geral

A estratégia combina duas abordagens:
- Mão Direita (heurística local) para exploração segura quando não há plano global válido.
- Planejamento por BFS (quando o objetivo é conhecido e o mapa já tem conectividade suficiente) para seguir o menor caminho descoberto.

A decisão final em cada passo é tomada por `Navigator::decidePlanned()`:
1) Se há um plano válido e a próxima direção absoluta é compatível com o ambiente (livre), converte-se para a ação relativa (frente/direita/esquerda/trás) e prioriza-se essa ação.
2) Caso contrário, seleciona-se entre as ações livres usando critérios de exploração e heurística (descritos abaixo), equivalendo a um fallback da regra da Mão Direita.

## Regra da Mão Direita (Right-Hand)

Implementada em `Navigator::decide()`:
- Prioriza: Direita > Frente > Esquerda > Trás (se todas bloqueadas).
- A pontuação (`score`) é calculada por `score_for()` usando pesos das heurísticas (`Heuristics`), penalizando direções bloqueadas.

Uso: quando não há plano válido (ou plano não aplicável no estado atual), `decidePlanned()` retorna `decide(sr)`.

## Observação de Paredes e Atualização do Mapa

`Navigator::observeCellWalls(cell, sr, heading)` converte leituras relativas (esquerda/frente/direita) para direções absolutas (N/E/S/W) com base em `heading` (0=N,1=E,2=S,3=W) e atualiza o `MazeMap` com paredes presentes/ausentes via `MazeMap::set_wall()`.

Também contabiliza visitas por célula em `seen_`, para favorecer exploração de áreas novas (menor contagem).

## Planejamento (BFS)

`Navigator::planRoute()` usa `Planner::bfs_path(map_, start_, goal_)` para obter uma rota do início ao objetivo no mapa conhecido. Em sucesso, o vetor `plan_` armazena a sequência de células do caminho (inclui início e objetivo).

Pré-requisitos:
- Dimensões do mapa definidas com `setMapDimensions(w,h)`.
- `setStartGoal(start, goal)` chamado para definir o objetivo.

## Decisão com Plano + Exploração

`Navigator::decidePlanned(current, heading, sr)` faz:
- Determina a direção absoluta desejada pelo plano da célula atual para a próxima (`N/E/S/W`).
- Constrói candidatos para cada direção livre relativa (Esquerda, Frente, Direita), computando:
  - Próxima célula (nx, ny) se for tomada aquela direção.
  - `seen[nx,ny]`: menor é melhor (0 é inédito; prioridade máxima).
  - `matches_plan`: se o movimento coincide com o passo desejado pelo plano.
- Ordenação estável dos candidatos por (na ordem):
  1) Inédito (`seen==0`) primeiro;
  2) Menor `seen` (menos visitado);
  3) Alinhamento com o plano (`matches_plan` verdadeiro);
  4) Maior pontuação heurística `score_for()`.
- Se não há candidatos (todas as laterais bloqueadas), retorna `Back`.

Efeito: mesmo com plano, o agente prefere explorar novidades e evitar loops, mas dá preferência a seguir o plano quando viável.

## Heurísticas e Reforço

`Heuristics` contém pesos para cada ação (direita/frente/esquerda/trás). A função `score_for()` transforma esses pesos em uma escala inteira [0..10], penalizando ações bloqueadas.

`Navigator::applyReward(action, reward)` atualiza os pesos via `update_heuristic(heur_, idx, reward)`, onde `idx` codifica a ação. Isso permite ajustes simples após episódios (ex.: reduzir colisões, preferir frente em corredores, etc.).

## Conversão entre Direções Absolutas e Relativas

O código utiliza `heading` absoluto (0=N,1=E,2=S,3=W) para converter entre:
- Relativas (Esquerda/Frente/Direita) e absolutas (`N/E/S/W`) ao observar paredes e ao avaliar candidatos.
- Absolutas do plano para ações relativas do robô (Frente/Direita/Esquerda/Trás).

Essa padronização evita inconsistências ao alternar entre planejamento e percepção local.

## Limitações Atuais

- O BFS usa o mapa conhecido; áreas não descobertas podem impedir um plano global até que passagens sejam observadas.
- A regra da Mão Direita é um heurístico simples; pode gerar trajetórias subótimas durante a exploração.
- A priorização por `seen_` reduz repetição, mas não substitui algoritmos de exploração mais ricos (ex.: fronteira ativa, D* Lite, etc.).

## Futuras Extensões

- Exibir o plano em tempo real no simulador (já há método `Navigator::currentPlan()` para visualização).
- Exploração guiada por fronteira e replanejamento incremental.
- Ajuste das heurísticas durante episódio (on-policy) e/ou por múltiplos episódios (off-policy) com recompensas mais informativas.

## Relacionado

- `NAVIGATOR.md` – detalhes de APIs, estruturas e integração com o simulador.
- `SIMULATOR.md` – UI, JSON do mapa e exportação da solução.
- `README.md` – visão geral, como compilar e executar.
