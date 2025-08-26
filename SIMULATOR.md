# SIMULATOR

Este documento descreve o simulador desktop opcional baseado em SDL2 localizado em `simulator/main.cpp`.

## Visão geral

- Renderiza um labirinto e um agente que executa navegação (heurística e/ou planejada) sobre o mapa `maze::MazeMap`.
- Permite gerar, salvar e carregar labirintos em `.maze` (conteúdo JSON) na pasta `maze/`.
- Interface simples com botões, lista de seleção de mapas e log lateral.
- O tempo de execução é exibido no título da janela e é congelado automaticamente quando o agente atinge o objetivo.
- Ao alcançar o objetivo, o simulador exporta a solução em `.soluct` versionado com metadados e métricas e também um registro de tentativa `.plan` com os passos executados.

Arquivos relacionados:
- Código: `simulator/main.cpp`
- Core de navegação e mapa: `src/core/Navigator.*`, `src/core/MazeMap.*`, `src/core/Learning.*`
- Planejamento: `src/core/Planner.*` (BFS)

## Compilação e execução

Pré-requisitos no host: SDL2 e opcionalmente SDL2_ttf.

```bash
cmake -B build-sim -S . -DBUILD_SIM=ON -DBUILD_TESTS=OFF -DBUILD_FIRMWARE=OFF
cmake --build build-sim --target simulator -j
./build-sim/simulator
```

Se SDL2/SDL2_ttf não forem encontrados, o alvo pode não ser criado ou será compilado sem renderização de textos.

## UI e Controles

- Botões:
  - Iniciar/Parar: alterna a execução do agente.
  - Novo Labirinto: gera um labirinto perfeito aleatório, salva em `maze/` e recarrega.
- Lista de seleção (quando aberta): mostra `maze/*.maze`. A primeira opção cria um labirinto novo e salva.
- Log lateral: mostra passos, colisões, custo, estado atual e mensagens.
- Atalhos de teclado:
  - R: reset do episódio atual (recomeça do início mantendo o labirinto).
  - Setas/WASD: navegação na lista de seleção quando ativa.

## Cronômetro (tempo)

- Variáveis: `start_ms`, `frozen_ms` e `time_frozen`.
- Comportamento:
  - Em execução: o tempo é `SDL_GetTicks() - start_ms`.
  - Ao atingir o objetivo: o tempo é congelado em `frozen_ms` e não avança mais (o título da janela reflete o valor congelado).
  - Em novo início/reset: `start_ms` é reinicializado e `time_frozen/frozen_ms` são limpos.

Este comportamento foi implementado para que a métrica de tempo represente o desempenho até o sucesso, sem continuar contando durante estados pós-sucesso.

## Estados principais do episódio

- Ready: aguardando início (botão Iniciar ativo).
- Running: agente executando.
- FinishedSuccess: objetivo alcançado; tempo congelado; é possível reiniciar.
- FinishedFail: falha (se aplicável); é possível reiniciar.

A transição para `FinishedSuccess` congela o tempo e pausa a simulação.

## Persistência de labirinto (formato e extensões)

- Pasta: `maze/` (criada automaticamente se não existir).
- Mapa `.maze` (conteúdo JSON) contém:
  - Tamanho (largura/altura), paredes por célula, início e objetivo.
  - Metadados: `name`, `email`, `github`, `date`.
- Metadados:
  - Podem ser preenchidos automaticamente via env: `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`.
  - Quando `SDL2_ttf` está disponível, um modal solicita esses dados caso ausentes.

## Exportação de solução (.soluct versionado)

- Arquivo: `maze/<mapa>_solution_<n>.soluct`, onde `<n>` incrementa apenas quando o conteúdo muda.
- Conteúdo inclui:
  - `map_file`, `width`, `height`
  - `entrance` (x, y, heading), `goal` (x, y)
  - `metrics`: `steps`, `collisions`, `time_s`, `cost`
  - `path`: lista ordenada de células `{x,y}` da rota final
  - `meta`: `name`, `email`, `github`, `date`

  Política de versionamento: se a nova solução é idêntica à última versão salva para o mesmo mapa, não cria um novo arquivo; caso contrário, incrementa o sufixo `<n>`.

## Exportação de plano/tentativa (.plan)

- Arquivo: `maze/<mapa>_attempt_<n>.plan` (JSON) por execução/episódio.
- Conteúdo inclui por passo: `from`, `to`, `heading_before`, `action`, `moved`, `event`, `collisions`, `delta_score`, `score_after`, `step_index`.
- Resumo: `result` (success/fail), `steps`, `collisions`, `score`, metadados e cabeçalho comum (`map_file`, dimensões, entrada/objetivo, `meta`).

## Solução de problemas

- "SDL2 not found; simulator target will not be built.": instale as dependências de desenvolvimento do SDL2.
- "SDL2_ttf not found; building simulator without text rendering": o simulador roda, mas rótulos de botões e textos não aparecem.

## Limitações atuais

- Rendering simples (wireframe); sem física contínua.
- Apenas estratégia Right-Hand e/ou caminho planejado via BFS.
- Resolução fixa da janela e layout de UI minimalista.

## Roadmap sugerido

- Pausar/retomar via teclado (P), e velocidade ajustável.
- Exportar replay ou métricas (CSV) por episódio.
- Melhorias de UX: foco/hover, tooltips, atalhos visuais.
