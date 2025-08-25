# RP2040 Maze Solver

<!-- Badges -->
[![CI](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/actions/workflows/ci.yml/badge.svg)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/actions/workflows/ci.yml)
![visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.Maze_Solver_RP2040)
[![License: CC BY-SA 4.0](https://img.shields.io/badge/License-CC_BY--SA_4.0-blue.svg)](https://creativecommons.org/licenses/by-sa/4.0/)
![Language: Portuguese](https://img.shields.io/badge/Language-Portuguese-brightgreen.svg)
[![Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/issues)
[![PRs](https://img.shields.io/github/issues-pr/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/pulls)
[![Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/stargazers)
[![Forks](https://img.shields.io/github/forks/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/network/members)
[![Watchers](https://img.shields.io/github/watchers/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/watchers)
[![Last Commit](https://img.shields.io/github/last-commit/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/commits)
[![Contributors](https://img.shields.io/github/contributors/ArvoreDosSaberes/Maze_Solver_RP2040)](https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/graphs/contributors)

Projecto de firmware, core, simulador e testes para um carrinho resolvedor de labirintos baseado em RP2040.

## Estrutura
- `firmware/`: loop principal RP2040 e integração HAL
- `src/core/`: navegação (`Navigator`), mapa (`MazeMap`), planejador (`Planner`), aprendizado (`Learning`), persistência (`PersistentMemory`)
- `src/hal/`: sensores IR e controle de motores
- `simulator/`: simulador simples (opcional, SDL2)
- `tests/`: testes unitários (Unity)
- `inc/Unity/`: framework de testes Unity (vendorizado)

Documento relacionado:
- `MotorControl_Implementations.md`: guia para criar novas implementações de `hal::MotorControl` (I2C/SPI/PWM...)

## Pré-requisitos (host)
- CMake >= 3.13
- Compilador C++17 (g++/clang++)

Para firmware RP2040, instale também o Pico SDK. Para o simulador, instale SDL2 (opcional).

## Compilar e executar os testes (host)
```bash
cmake -B build-tests -S . -DBUILD_TESTS=ON -DBUILD_FIRMWARE=OFF -DBUILD_SIM=OFF
cmake --build build-tests --target all -j

# Executando diretamente os binários de teste
./build-tests/planner_tests
./build-tests/random_maze_tests
./build-tests/maze_tests
./build-tests/navigator_planned_tests
./build-tests/learning_tests
./build-tests/reach_goal_tests
```

Dica: CTest está registrado no `CMakeLists.txt`, mas em alguns ambientes pode não listar automaticamente. Se preferir tentar:
```bash
ctest --test-dir build-tests -V
```
Se não aparecerem testes, execute os binários diretamente como acima.

## O que os testes validam
- `planner_tests`: BFS básico em mapas simples
- `random_maze_tests`: BFS encontra caminho em labirintos perfeitos aleatórios
- `maze_tests` e `navigator_planned_tests`: decisões do `Navigator`
- `learning_tests`: em 2 labirintos (seeds) o custo do 2º episódio é ≤ ao 1º
- `reach_goal_tests`: agente alcança o objetivo em 4 labirintos aleatórios

## Compilar o simulador (opcional)
Requer SDL2 no sistema.
```bash
cmake -B build-sim -S . -DBUILD_SIM=ON -DBUILD_TESTS=OFF -DBUILD_FIRMWARE=OFF
cmake --build build-sim --target simulator
./build-sim/simulator
```

Se o CMake não encontrar SDL2, o alvo não será criado. Instale `libsdl2-dev` (Linux) ou equivalente.

## Compilar o firmware (RP2040)
Requer Pico SDK configurado no sistema.
```bash
cmake -B build-fw -S . -DBUILD_FIRMWARE=ON -DBUILD_TESTS=OFF -DBUILD_SIM=OFF
cmake --build build-fw --target rp2040_maze_solver
```
O binário UF2 estará em `build-fw/` (nome padrão gerado pelo Pico SDK). Parâmetros (overrides sugeridos) podem ser passados via `-D` no CMake (veja `CMakeLists.txt`, seções `CFG_*`).

## Persistência (host e RP2040)
- Host: heurísticas salvas em `~/.rp2040_maze/heuristics.bin` por `PersistentMemory`.
- RP2040: heurísticas gravadas no último setor de flash (4 KB) usando `hardware/flash.h`, com cabeçalho `{magic, version, size}` para integridade.

Configuração do tamanho total de flash (para posicionar o setor de persistência):
```bash
# 2 MB (padrão)
cmake -B build-fw -S . -DBUILD_FIRMWARE=ON -DPMEM_FLASH_TOTAL_BYTES=2097152

# 4 MB
cmake -B build-fw -S . -DBUILD_FIRMWARE=ON -DPMEM_FLASH_TOTAL_BYTES=4194304
```

Comandos de boot (USB CDC, janela ~3s):
- `RESET`/`R`: apaga o setor de persistência
- `STATUS`: mostra `saved_count` e `active_profile`

## Parametrização (macros CFG_*)
Alguns parâmetros podem ser ajustados via opções CMake (passadas com `-D`):
- `CFG_TARGET_SPEED_CM_S` (float) velocidade alvo de cruzeiro
- `CFG_MOTOR_*` pinos e inversões dos motores (por exemplo `CFG_MOTOR_LEFT_PWM_PIN`, `CFG_MOTOR_RIGHT_PWM_PIN`)
- `CFG_IR_ADC_*` canais e filtros dos sensores IR

Exemplo:
```bash
cmake -B build-fw -S . \
  -DBUILD_FIRMWARE=ON \
  -DCFG_TARGET_SPEED_CM_S=12.0 \
  -DCFG_MOTOR_LEFT_PWM_PIN=4 -DCFG_MOTOR_RIGHT_PWM_PIN=5
```

## Documentação (Doxygen)
Gerar documentação local (se `doxygen` estiver instalado):
```bash
doxygen Doxyfile
```
Saída em `docs/index.html`. Alternativamente via CMake:
```bash
cmake -S . -B build
cmake --build build --target docs -j
# Abra: docs/index.html
```

## Versão
Versão atual: **v0.0.1**.

- Notas de versão: veja `CHANGELOG.md`
- Release no GitHub: https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/releases/tag/v0.0.1

## Contribuindo
- Abra issues descrevendo bug/feature.
- Faça fork, crie uma branch, edite `PLAN.md` com suas propostas e abra um Pull Request explicando as mudanças.
- Padrões:
  - Testes: adicione casos em `tests/` quando alterar comportamento.
  - Estilo: C++17, cabeçalhos com comentários Doxygen.
  - CI (opcional): rodar `ctest` localmente antes do PR.

## Logs e comandos (firmware)
Na inicialização, há janela para comandos via USB CDC. Exemplo de sequência:
1. Envie `STATUS` para checar se há heurísticas salvas.
2. Envie `RESET` para apagar.
3. Após atingir o objetivo, as heurísticas são salvas automaticamente; reinicie e confira `STATUS` e logs de carga.

## Licença
Este projeto é licenciado sob a licença Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0).

- Texto da licença: LICENSE
- Resumo: https://creativecommons.org/licenses/by-sa/4.0/

