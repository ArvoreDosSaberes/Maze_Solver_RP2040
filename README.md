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
- [PLAN.md](PLAN.md): planejamento e roadmap de versões
- [REFERENCE.md](REFERENCE.md): referência geral do projeto
- [NAVIGATOR.md](NAVIGATOR.md): visão detalhada do sistema de navegação, planejamento (BFS) e heurísticas
- [STRATEGIA.md](STRATEGIA.md): explica a estratégia atual de busca da saída (Mão Direita + planejamento BFS + exploração)
- [SIMULATOR.md](SIMULATOR.md): detalhes de funcionamento da UI, estados, tempo/cronômetro e formatos de arquivo do simulador (.maze/.soluct/.plan)
- [MotorControl_Implementations.md](MotorControl_Implementations.md): guia para criar novas implementações de `hal::MotorControl` (I2C/SPI/PWM...)
- [CHANGELOG.md](CHANGELOG.md): notas de versão
- [LICENSE](LICENSE): texto da licença (CC BY-SA 4.0)

## Pré-requisitos (host)
- CMake >= 3.13
- Compilador C++17 (g++/clang++)

Para firmware RP2040, instale também o Pico SDK. Para o simulador, instale SDL2 (opcional) e, para rótulos/textos na UI, SDL2_ttf (opcional, recomendado).

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
Requer SDL2 no sistema. Para renderização de textos (rótulos de botões, log lateral e modal de metadados), instale SDL2_ttf.
```bash
cmake -B build-sim -S . -DBUILD_SIM=ON -DBUILD_TESTS=OFF -DBUILD_FIRMWARE=OFF
cmake --build build-sim --target simulator
./build-sim/simulator
```

Se o CMake não encontrar SDL2, o alvo não será criado. Instale `libsdl2-dev` (Linux) ou equivalente. Para textos na UI, instale também `libsdl2-ttf-dev`.

### Recursos do simulador (UI, arquivos e seleção)

- Sidebar (lateral direita) com log de eventos e métricas (passos, colisões, custo).
- Botões: "Iniciar/Parar" e "Novo" (gera labirinto aleatório, salva e reinicia).
- Salvar/carregar labirintos em `.maze` (conteúdo JSON) na pasta `maze/` (criada automaticamente).
- Exporta a solução ao alcançar o objetivo: salva `maze/<mapa>_solution_<n>.soluct` com metadados, métricas e o caminho (versionado; não cria nova versão se o conteúdo não mudou).
- Exporta o plano/tentativa por passo: salva `maze/<mapa>_attempt_<n>.plan` contendo o log por passo (ação, moveu, colisões, score), status (success/fail) e resumo.
- Menu/seleção simples: lista `maze/*.maze` em ordem alfabética para abrir; opção de gerar labirinto aleatório e salvar.
- Metadados salvos no arquivo: `name`, `email`, `github`, `date`.
  - Pré-preenchimento por variáveis de ambiente: `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`.
  - Se ausentes e `SDL2_ttf` presente, um modal interativo solicita os dados (uma vez por sessão). Sem `SDL2_ttf`, usa valores padrão das variáveis de ambiente.
- Utilitários de filesystem criam `maze/` quando necessário.

Comportamentos recentes implementados:
- O cronômetro congela automaticamente ao alcançar o objetivo (o título da janela para de avançar).
- O botão "Novo Labirinto" permanece habilitado após reset e após carregamento de mapa.

Controles básicos: setas/WASD para navegar entre opções do menu (quando aplicável) e iniciar. Execução gráfica mostra paredes (verde) e agente (vermelho).

#### Solução de problemas (SDL2/SDL2_ttf)

- Mensagem: `SDL2 not found; simulator target will not be built.`
  - Debian/Ubuntu: `sudo apt-get install libsdl2-dev`
  - Fedora: `sudo dnf install SDL2-devel`
  - Arch: `sudo pacman -S sdl2`

- Mensagem: `SDL2_ttf not found; building simulator without text rendering`
  - Debian/Ubuntu: `sudo apt-get install libsdl2-ttf-dev`
  - Fedora: `sudo dnf install SDL2_ttf-devel`
  - Arch: `sudo pacman -S sdl2_ttf`
  - Efeito: o simulador executa, porém rótulos e textos não aparecem.

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
Versão atual: **v0.0.3**.

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

