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
 - [x] Simulador: salvar/carregar labirintos em `.maze` (conteúdo JSON), exportar solução `.soluct` e plano `.plan`, e seleção de labirinto via menu.

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
### Versão 0.0.1
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
- [x] Simulador: helpers para serializar/deserializar labirinto em JSON (dimensões, paredes, entrada, objetivo e metadados) e utilitários de filesystem (`maze/`).
 - [x] Simulador: coleta de metadados via variáveis de ambiente (`GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`) ou entrada interativa ao salvar JSON.
 - [x] Simulador: menu simples para escolher labirinto JSON existente em `maze/` ou gerar aleatório e salvar.
- [x] CI/CD: Adicionar workflow de CI (GitHub Actions) para compilar testes e rodar CTest.
- [x] GIT: Adicionar os arquivos por grupos de funcionalidade e criar os comiits documentando.
- [x] GIT: Ajustar o PLAN.md E README.md para refletir as alterações de versão.

   - fazer o merge para o main
   - fazer o push para o repositório.
### Versão 0.0.3
- [x] Adicionar frame lateral com informações do carrinho e suas decisões como é feito no terminal
- [x] Adicionar um novo botão para iniciar a solução do labirinto, outro para gerar um novo labirinto aleatório e salvar.
- [x] Persistência com novas extensões: `.maze` (mapa), `.soluct` (solução) e `.plan` (tentativa por passo) com versionamento.
  - Observação: requer `SDL2_ttf` para exibir textos/labels; sem ele, o simulador funciona, porém sem rótulos.
  - Modal de metadados (uma vez por sessão) quando `SDL2_ttf` está presente; caso contrário, usa variáveis de ambiente (`GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`).
- [X] Carrinho deve demonstrar o planejamento em tempo real

### Próximos passos versão 0.0.4
- [ ] Apresentar no simulador estatísticas do processo de descoberta da rota: tentativas mal sucedidas e bem sucedidas, pontuação obtida e ideal, iterações realizadas e ideais. Incluir dados do labirinto: dimensões, número de interseções e curvas.

### Próximos passos versão 0.1.0
- [ ] Permitir troca de estratégia de planejamento entre RightHand e A*; criar uma interface para facilitar a troca e armazenar, em `src/core/navigator/`, os algoritmos de navegação criados.
- [ ] Implementar A* (Dijkstra/A*) para rota ótima.
- [ ] Evoluir o componente de aprendizado para ajustar pesos/heurísticas com base em métricas coletadas.

### Próximos passos versão 0.2.0
- [ ] Tornar os sensores flexíveis para que possam ser trocados facilmente. devem ser armazenados em hal/sensors os possíveis tipos de sensores, como array para detectar parede e frente, o array de 5 sensores para detectar linha para carinho seguir de linha.

## Próximos passos (curto prazo)
Consolidado até "Versão 0.0.3". Planejamento em andamento nas seções de v0.0.4 e v0.1.0.

## Plano de Microcommits
- Objetivo: reduzir risco e facilitar revisão. Cada item abaixo deve resultar em 1 commit pequeno, com mensagem clara no imperativo.

### v0.0.4 – Estatísticas no simulador
- [ ] Adicionar estrutura `RunStats` no simulador (acertos, falhas, passos, pontuação atual/ideal).
- [ ] Expor APIs para atualizar `RunStats` a partir do `Navigator` (hooks de evento).
- [ ] Renderizar painel de estatísticas vazio (wireframe) no frame lateral existente.
- [ ] Preencher métricas básicas: dimensões do labirinto e contagem de interseções/curvas.
- [ ] Persistir resumo de `RunStats` em `.plan` (campo `summary`).
- [ ] Incluir botão/tecla para reset das estatísticas da sessão.

### v0.1.0 – Estratégias de planejamento
- [ ] Criar enum `PlanningStrategy { RightHand, AStar }` em `src/core`.
- [ ] Criar interface `INavigatorStrategy` com `decideNextMove()` e dependências mínimas.
- [ ] Adaptar `RightHandStrategy` a partir da lógica atual do `Navigator`.
- [ ] Implementar `AStarStrategy` (estrutura esqueleto, sem heurística primeiro).
- [ ] Integrar seleção de estratégia via config (CLI/env no simulador e macro no firmware).
- [ ] Exibir estratégia ativa no painel lateral do simulador.

### v0.1.0 – A* incremental
- [ ] Implementar heurística Manhattan e custo básico.
- [ ] Integrar A* com `MazeMap` e `Planner` existentes.
- [ ] Adicionar teste unitário para A* em labirinto simples.
- [ ] Adicionar fallback automático para RightHand se A* falhar.

### v0.2.0 – Abstrações de sensores
- [ ] Criar diretório `src/hal/sensors/` e mover `IRSensorArray` para lá.
- [ ] Definir interface `ISensorArray` e padronizar leitura normalizada.
- [ ] Implementar drivers: IR parede/frente e (stub) 5 sensores de linha.
- [ ] Adicionar seleção de sensor via CMake e macros (`CFG_SENSOR_BACKEND`).
- [ ] Atualizar testes para cobrir múltiplos backends.
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

### Observações do simulador (SDL2 e arquivos)

- SDL2 é necessário no host para compilar o alvo `simulator`. Em distribuições Debian/Ubuntu, instale com: `sudo apt-get install libsdl2-dev`.
- O simulador cria/usa a pasta `maze/` automaticamente quando necessário.
- Menu do simulador:
  - Lista arquivos `maze/*.maze` em ordem alfabética para seleção.
  - Opção de gerar um labirinto aleatório e salvar como `.maze` (JSON) com metadados.
- Extensões e formatos:
  - Mapas: `.maze` (conteúdo JSON)
  - Soluções: `.soluct` (JSON) – versionado sem duplicar conteúdo
  - Tentativas: `.plan` (JSON) com log por passo e resumo – versionado
- Metadados: `name`, `email`, `github`, `date`.
  - Podem ser preenchidos via variáveis de ambiente: `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`.
  - Se ausentes e `SDL2_ttf` estiver disponível, o simulador solicita em um modal; caso contrário, usa valores padrão.

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

Desenvolva um firmware para um carrinho que soluciona labirintos. Ele deve buscar a melhor saída do labirinto, armazenando em sua memória os labirintos solucionados para reutilização em novas tentativas. Inicialmente, usar o algoritmo da mão direita (prioriza sempre a saída à direita); depois, aprimorar táticas com novos algoritmos de solução. Crie classes que abstraiam o acesso às portas PWM para controle dos motores; o carrinho tem dois motores que controlam as rodas para definir a direção. Crie uma classe que abstraia o acesso aos sensores de obstáculos analógicos infravermelhos reflexivos para identificar as opções de saída (esquerda, frente e direita). Faça testes unitários que abram uma interface gráfica que representa o labirinto a ser solucionado de forma aleatória; teste 4 labirintos aleatórios e repita dois para constatar o melhoramento do algoritmo. A interface gráfica deve representar o labirinto com paredes em verde e o carrinho como um circuito vermelho. Use o framework Unity que está em `inc/Unity`. Apresente o plano para aprovação e faça os ajustes necessários no `CMakeLists.txt` para adicionar as bibliotecas e dependências do Unity.

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
- Geração de release v0.0.3 com novas extensões (.maze/.soluct/.plan), versionamento de arquivos e logging por passo no simulador.
- Atualizar README/SIMULATOR/CHANGELOG para refletir formatos e recursos atuais.
- (Opcional) Publicar `docs/` via GitHub Pages e manter artifact de Doxygen no CI.