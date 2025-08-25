# RP2040 Maze Solver – Plano e Requisitos

Documento vivo para guiar o desenvolvimento do firmware, testes e simulador do carrinho solucionador de labirintos.

## Objetivo
Construir um sistema embarcado (RP2040) que:
- Encontre saída de labirintos, começando com estratégia “Mão Direita”.
- Construa um mapa e, após explorar, use rota ótima (flood fill/A*).
- Aprenda/ajuste heurísticas ao longo de execuções e persista resultados.
- Ofereça testes unitários (Unity) e simulador gráfico (SDL2) no host.

## Requisitos Funcionais
- [x] Estratégia inicial Right-hand como fallback.
- [x] Construção de mapa do labirinto durante exploração.
- [x] Planejamento de rota ótima (flood fill/A*).
- [x] Persistência de heurísticas e mapas:
  - [x] RP2040: heurísticas em flash (hardware_flash) com cabeçalho de integridade.
  - [x] Host: arquivos (ex.: ~/.rp2040_maze/).
  - [x] Snapshot de mapa (RP2040/host).
- [x] Componente de aprendizado simples (pesos/heurísticas) aplicado às decisões.
- [x] Reutilizar conhecimento entre execuções e demonstrar melhora ou estabilidade.
- [x] Simulador gráfico com paredes verdes e carrinho vermelho.
- [x] Testes com 4 labirintos aleatórios; repetir 2 e verificar melhoria/igual.

## Requisitos Não Funcionais
- Código organizado por camadas: core, hal, firmware, simulator, tests.
- Build configurável via CMake (firmware/testes/simulador opcionais).
- Fail-safes mínimos no controle (validação sensores, clamps, stop em obstáculo).
- Configuração por macros CMake para pinos/canais e parâmetros.

## Arquitetura / Pastas
- `firmware/`: entrada RP2040 (`main.cpp`) e loop de controle.
- `src/core/`: lógica de navegação e suporte (Navigator, MazeMap, Planner, Learning, PersistentMemory).
- `src/hal/`: abstrações de hardware (motores via ponte H, sensores IR ADC).
- `simulator/`: simulador host (SDL2) para visualização e métricas.
- `tests/`: testes Unity.
- `inc/Unity/`: framework de testes Unity (vendorizado).

## Itens Implementados
- [x] `Navigator` com estratégia Right-hand, integração com `MazeMap`/`Planner` (BFS) e pontuação com `Heuristics`.
- [x] HAL de motores (ponte H) e sensores IR (ADC + EMA).
- [x] Fail-safes no controle (firmware `main.cpp`).
- [x] CMake com opções: `BUILD_FIRMWARE` (ON), `BUILD_TESTS` (OFF), `BUILD_SIM` (OFF).
- [x] Teste Unity básico para Right-hand.
- [x] `PersistentMemory` stubs (status/erase) + API heurísticas (save/load) stub in-memory.
- [x] Esqueleto de simulador SDL2 (só janela/desenhos básicos).
- [x] `MazeMap.hpp`: grid e paredes bidirecionais com `set_wall()`.
- [x] `Planner.hpp`: BFS/flood fill inicial com `Planner::bfs_path()`.
- [x] Testes do planner (`tests/test_planner.cpp`) e separação de executáveis de teste no CMake (`maze_tests` e `planner_tests`) para evitar múltiplos `main()`).
- [x] Firmware: integração completa do `Navigator` (observeCellWalls/planRoute/decidePlanned), reforço com `applyReward()` e persistência de heurísticas no goal.
- [x] Doxygen básico configurado (OUTPUT para `docs/`, alvo CMake `docs`).
- [x] Unity excluído da indexação do Doxygen (`EXCLUDE = inc/Unity`).
- [x] Licença definida para CC BY-SA 4.0 e arquivo `LICENSE` adicionado.

## Próximas Entregas
1. Mapa e Planejador
   - [x] `MazeMap.{hpp}`: grid, paredes, bidirecionalidade, bounds.
   - [x] `Planner.{hpp}`: BFS/flood fill para rota.
2. Persistência real
   - [x] RP2040: layout simples de flash para heurísticas (1 setor, header {magic,ver,size}).
   - [x] RP2040: snapshot do mapa (2ª página do setor reservado; header {magic,ver,w,h,size}).
   - [x] Host: arquivos binários em pasta do usuário.
   - [x] Firmware: carrega snapshot no boot e salva ao atingir goal.
3. Aprendizado
   - [x] `Learning.hpp` com heurísticas e update rule.
   - [x] Aplicado no `Navigator` com recompensas; salvar heurísticas via `PersistentMemory` ao atingir goal.
4. Testes Unity
   - [x] `test_planner.cpp`: valida BFS básico (mapa aberto e bloqueios).
   - [x] Aleatoriedade: labirintos pequenos gerados randômicamente.
   - [x] `test_learning.cpp`: repetir 2 labirintos e verificar melhoria/igual.
   - [x] Teste geral: agente alcança o objetivo em 4 labirintos aleatórios.
   - [x] `test_persistence_map.cpp`: snapshot de mapa (save/load e mismatch de dimensões).
5. Simulador
   - [x] Renderização de labirinto (paredes verdes) e agente (vermelho) percorrendo rota.
   - [x] Métricas: passos, tempo, colisões, custo.
6. Documentação
   - [x] Doxygen: gerar documentação de API (Doxyfile versionado; saída em `docs/`).
   - [x] REFERENCE.md: lista de referências (micromouse, RP2040 SDK, Unity, SDL2, algoritmos).
   - [x] Documentação do código: comentários Doxygen nas classes de `src/core/` e `src/hal/` (concluído).
   - [x] Documentação do firmware: fluxo do `firmware/main.cpp`, comandos e parâmetros `CFG_*` (feito).
   - [x] Documentação do simulador: como compilar/rodar, dependências SDL2 e controles.
   - [x] Documentação dos testes: como rodar binários e via CTest.
   - [x] Documentação do plano: orientar contribuição via edição de `PLAN.md`, PRs e issues.
   - [x] README: parametrização do carrinho (macros `CFG_*`), execução de testes, build de firmware/simulador, dependências.
   - [x] Adicionar badges conforme a lista abaixo:
     - Contador de visitas: <img src="https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.Maze_Solver_RP2040" />
     - [![License: CC BY-SA 4.0](https://img.shields.io/badge/License-CC_BY--SA_4.0-blue.svg)](https://creativecommons.org/licenses/by-sa/4.0/)
     - ![Language: Portuguese](https://img.shields.io/badge/Language-Portuguese-brightgreen.svg)
     - entre outros badges sobre o CI/CD
     - [![GitHub issues](https://img.shields.io/github/issues/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/issues)
     - [![GitHub pull requests](https://img.shields.io/github/issues-pr/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/pulls)
     - [![GitHub stars](https://img.shields.io/github/stars/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/stargazers)
     - [![GitHub forks](https://img.shields.io/github/forks/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/network/members)
     - [![GitHub watchers](https://img.shields.io/github/watchers/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/watchers)
     - [![GitHub license](https://img.shields.io/github/license/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/blob/main/LICENSE)
     - [![GitHub last commit](https://img.shields.io/github/last-commit/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/commits)
     - [![GitHub contributors](https://img.shields.io/github/contributors/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/graphs/contributors)
7. CI/CD
  - [x] Adicionar workflow de CI (GitHub Actions) para compilar testes e rodar CTest.
8. GIT
   - Adicionar os arquivos por grupos de funcionalidade e criar os comiits documentando.
   - Ajustar o PLAN.md E README.md para refletir as alterações de versão.
   - Gerar a versão 0.0.1
   - fazer o merge para o main
   - fazer o push para o repositório.

## Próximos passos (curto prazo)
- [x] Adicionar e versionar `Doxyfile` com configuração mínima e alvo `docs` no CMake.
- [x] Completar comentários Doxygen nos headers do core/hal mais usados (Fase 1).
- [x] Documentar fontes `.cpp` correspondentes no core/hal (Fase 2).
- [x] Documentar macros/constantes do `firmware/main.cpp` (Fase 3).
- [x] Documentar `simulator/` e testes (Fase 4).
- [x] Incluir seção “Contribuindo” no `README.md` com padrão de PRs e issues.
- [x] Adicionar workflow de CI (GitHub Actions) para compilar testes e rodar CTest.
## Build Targets e Opções (CMake)
- Opções:
  - `-DBUILD_FIRMWARE=ON|OFF`
  - `-DBUILD_TESTS=ON|OFF`
  - `-DBUILD_SIM=ON|OFF`
- Firmware RP2040: Pico SDK apenas quando `BUILD_FIRMWARE=ON`.
- Testes/Simulador (host): toolchain nativa; Unity/SDL2 opcionais.

Exemplos de uso:
```bash
# Testes (host)
cmake -B build-test -S . -DBUILD_TESTS=ON -DBUILD_FIRMWARE=OFF -DBUILD_SIM=OFF
cmake --build build-test -j
ctest --test-dir build-test -V

# Simulador
cmake -B build-sim -S . -DBUILD_SIM=ON -DBUILD_TESTS=OFF -DBUILD_FIRMWARE=OFF
cmake --build build-sim -j

# Firmware (RP2040)
cmake -B build -S . -DBUILD_FIRMWARE=ON -DBUILD_TESTS=OFF -DBUILD_SIM=OFF
cmake --build build -j
```

## Configuração por Macros (exemplos)
- Motores: `CFG_MOTOR_*` (pinos, pwm, inversões, etc.).
- Sensores IR: `CFG_IR_ADC_*` (canais ADC e parâmetros de filtro).
- Velocidades/limiares: `CFG_TARGET_SPEED_CM_S`, etc.

## Métricas de Sucesso
- Encontra saída com Right-hand em labirintos simples.
- Após explorar, gera rota ótima e a segue corretamente.
- Persistência funcionando: carrega heurísticas/mapa e melhora/estabiliza desempenho.
- Testes Unity passam em host e simulador exibe trajetórias corretas.

## Notas de Manutenção
- Este arquivo deve ser atualizado a cada entrega concluída (marcar checkboxes) e quando escopo/decisões mudarem.

Desenvolva um firmware para um carrinho que soluciona labirintos, ele deve buscar a melhor saida do labiringo, armazenando em sua memória os labrintos solucionados de forma que possa reutilizar em novas tentativas de solução, inicialmente ele deve usar o algorimo da mão direita onde ele sempre tentará primeiro a opção de saida a direita, em seguida ele deve ir melhorando as táticas com novos algorítimos de solução, crie classes que abstraia o acesso as portas PWM para controle dos motores, o carrinho tem dois motores para controlar as rodas de forma a definir a direção, crie uma classe que abstraia o acesso aos sensores de obstáculos analógicos ifravermelhos reflexivos para identificar as opções de saida, a esquerda, frente e direita, faça testes unitários que abra uma interface gráfica que representa o labirinto a ser solucionado de forma aleatóiria teste 4 labirintos aleatóris e repita dois para constatar o melhoramento do algortimo genético. a interface gráfica deve representar o labirinto com suas paredes em verde e o carrinho como um circuito vermelho. use o framework unity que está na biblioteca /inc/unity. apresente o plano para eu aprovar ou fazer alterçaões. faça os ajustes necessários no CMakeFiles.txt para adicionar as bibliotecas e as dependências necessárias para o unity.

## Estado atual da documentação (Doxygen)

- [x] `src/core/MazeMap.hpp`
- [x] `src/core/Learning.hpp`
- [x] `src/core/Navigator.hpp`
- [x] `src/core/PersistentMemory.hpp`
- [x] `src/hal/IRSensorArray.hpp`
- [x] `src/core/Planner.hpp`
- [x] `src/core/PersistentMemory.cpp` (constantes, macros, helpers e métodos)
- [x] `src/core/Navigator.cpp` (métodos e helpers)
- [x] `src/hal/IRSensorArray.cpp` (construtor, helper ADC, `readAll()`)
- [x] `src/hal/MotorControl.hpp`
- [x] Implementações `src/hal/h_bridge/*`
- [x] `firmware/main.cpp` macros `CFG_*`, callback e fluxo (feito)
- [x] `simulator/` e `tests/` (descrições e uso) (feito)

3. Fase 3 – Firmware
   - Concluída. `firmware/main.cpp` documentado (macros `CFG_*`, callbacks/loop, comandos, integração `Navigator`/`PersistentMemory`).

4. Fase 4 – Simulador e Testes
   - Concluída. Simulador e testes documentados (execução, dependências, propósito por arquivo).

### Convenções Doxygen
- Cabeçalho por arquivo: `@file`, `@brief`, detalhes, autor, `@since`.
- Em classes/métodos: `@brief`, `@param`, `@return`, `@throws` (se aplicável), exemplos curtos.
- Para macros/constantes públicas: explicar unidade, faixa, efeito no sistema.
- Preferir `@copydoc` em .cpp quando a assinatura está no .hpp.
- Manter nomes totalmente qualificados (ex.: `maze::Navigator`).

### Definition of Done (DoD)
- Build `docs` sem erros e sem avisos novos.
- Cobertura: símbolos públicos documentados nas pastas alvo; itens internos podem ser resumidos.
- README, PLAN e REFERENCE atualizados quando necessário.

### Próximas ações
- Geração de release v0.0.1 e fluxo GIT: commits por funcionalidade, atualização de versão em README/PLAN, merge em `main` e push (incluindo tag).
- (Opcional) Publicar `docs/` via GitHub Pages e manter artifact de Doxygen no CI.