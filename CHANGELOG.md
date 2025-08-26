# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

## [0.0.2] - 2025-08-26
### Added
- Simulator: JSON load/save helpers (dimensions, walls, entrance, goal, metadata).
- Simulator: metadata capture via env vars `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE` and, when SDL2_ttf is present, interactive modal (one per session).
- Simulator: filesystem helpers to ensure `maze/` and `make/` directories and list `maze/*.json`.
- Simulator: simple menu to select an existing JSON maze or generate a random one and save.
- Simulator UI: sidebar with event log/metrics and buttons (Iniciar/Parar, Novo).

### Changed
- Documentation: updated `PLAN.md` and `README.md` with simulator JSON features, environment variables, SDL2/SDL2_ttf troubleshooting, and UI description.
- CMake: robust SDL2_ttf detection (Find module variants, pkg-config fallback) and linking; define `HAVE_SDL_TTF` only when includes+libs are present.

### Fixed
- `simulator/main.cpp`: missing declarations and conditional compilation paths when SDL2_ttf is absent.
- Linker errors due to missing `SDL2_ttf` linkage.

### Notes
- Build: simulator target requires SDL2. Text labels require SDL2_ttf.

## [0.0.1] - 2025-08-25
### Added
- Firmware `firmware/main.cpp` com laço de controle periódico, comandos de boot (USB CDC), integração `Navigator`/`PersistentMemory`, HAL de motores e sensores IR.
- Core de navegação em `src/core/` (Navigator, MazeMap, Planner, Learning, PersistentMemory).
- HAL em `src/hal/` com `IRSensorArray` (ADC+EMA) e `MotorControl` (ponte H).
- Simulador em `simulator/` (SDL2) com renderização de labirinto e agente, e coleta de métricas.
- Testes Unity em `tests/` cobrindo planner, navegação (Right-hand/planned), aprendizado, persistência e metas.
- Documentação Doxygen (alvo `docs`) e comentários detalhados nas principais unidades.
- CI (GitHub Actions) para build, `ctest` e geração de Doxygen (artefato anexado).

### Changed
- `README.md` atualizado com instruções de build, execução, badges (inclui status do CI) e seção de contribuição.
- `PLAN.md` atualizado para refletir estado atual, fases concluídas e próximos passos operacionais de release.

### Notes
- Firmware build requer Pico SDK configurado. Simulador requer SDL2.

[0.0.1]: https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/releases/tag/v0.0.1
[0.0.2]: https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/releases/tag/v0.0.2
