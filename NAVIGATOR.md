# NAVIGATOR

Este documento descreve o sistema de navegação e descoberta de melhor caminho implementado no núcleo do projeto.

Arquivos-chave:
- Navegador: `src/core/Navigator.hpp` e `src/core/Navigator.cpp`
- Mapa: `src/core/MazeMap.hpp`
- Aprendizado/Heurística: `src/core/Learning.hpp`
- Planejador (BFS): `src/core/Planner.*`

## Conceitos

- Células em grade com paredes N/E/S/W: `maze::MazeMap` e `maze::Cell`.
- Posição e orientação do agente: posição (`maze::Point`) e heading absoluto 0..3 (0=N,1=E,2=S,3=W).
- Leitura de sensores (simulada/real): `SensorRead { left_free, front_free, right_free }` indica se há passagem livre nas direções relativas.
- Ações possíveis: `Action { Right, Forward, Left, Back }`.
- Heurística: `Heuristics { w_right, w_front, w_left, w_back }` influencia preferências.

## Atualização do mapa a partir de sensores

Função: `Navigator::observeCellWalls(Point cell, const SensorRead& sr, uint8_t heading)`
- Converte aberturas relativas para paredes absolutas N/E/S/W conforme a orientação atual.
- Chama `MazeMap::set_wall()` para registrar paredes bidirecionais.

Trecho relevante (`src/core/Navigator.cpp`):
- `observeCellWalls()` linhas 64–80 mapeia esquerda/frente/direita para N/E/S/W, e usa `map_.set_wall()`.

## Estratégias de decisão

1) Heurística Right-Hand
- Função: `Navigator::decide(const SensorRead& sr)`
- Lógica: prioriza direita → frente → esquerda; caso todas fechadas, retorna trás.
- A pontuação é dada por `score_for(Action, SensorRead)`, que mapeia pesos 0.2–3.0 para uma escala simples 0..10, penalizando direções bloqueadas.

2) Caminho planejado (BFS)
- Função: `Navigator::planRoute()` utiliza `Planner::bfs_path(map_, start_, goal_)` para obter uma sequência de pontos do início ao objetivo.
- Função: `Navigator::decidePlanned(Point current, uint8_t heading, const SensorRead& sr)`
  - Se um plano está disponível e consistente com a posição atual, converte a próxima direção absoluta desejada em ação relativa (Forward/Right/Left/Back) com base no heading atual.
  - Caso o plano seja inválido ou ausente, recorre a `decide(sr)` (heurística Right-Hand).

## Aprendizado por reforço simples (on-line)

- Função: `Navigator::applyReward(Action a, float reward)`
  - Converte a ação em índice 0..3 e chama `update_heuristic(Heuristics&, uint8_t, float)`.
- Função: `update_heuristic()` em `Learning.hpp`
  - Ajusta o peso da ação com uma taxa de aprendizado fixa e satura na faixa [0.2, 3.0].
  - Efeito: decisões futuras tendem a repetir ações recompensadas positivamente.

## Estados, início e objetivo

- `Navigator` mantém `start_`, `goal_` e `plan_`.
- Para navegação planejada, é necessário definir um objetivo e chamar `planRoute()`; caso bem-sucedido, `plan_` é preenchido e `decidePlanned()` tenta seguir o caminho.
- Quando o agente atinge o objetivo, componentes de mais alto nível (simulador ou firmware) podem aplicar recompensa positiva e/ou persistir heurísticas.

## Interação com o simulador

- O simulador (`simulator/main.cpp`) usa `Navigator` para:
  - Observar paredes com base em sensores sintéticos.
  - Decidir ações (heurística ou planejado via BFS).
  - Aplicar recompensas simples (por exemplo, penalizar colisões, recompensar progresso/sucesso).
- O tempo do episódio congela ao sucesso no simulador, evidenciando o desempenho até alcançar o objetivo.

## Extensões sugeridas

- Outras estratégias: Left-Hand, Follow-Wall adaptativo, D* Lite para replanejamento dinâmico.
- Recompensas densas: distância ao objetivo, penalização por loops.
- Integração com persistência de heurísticas por perfil/seed.
