# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

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
