/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "RP2040 Maze Solver", "index.html", [
    [ "Estrutura", "index.html#autotoc_md1", null ],
    [ "Pré-requisitos (host)", "index.html#autotoc_md2", null ],
    [ "Compilar e executar os testes (host)", "index.html#autotoc_md3", null ],
    [ "O que os testes validam", "index.html#autotoc_md4", null ],
    [ "Compilar o simulador (opcional)", "index.html#autotoc_md5", [
      [ "Recursos do simulador (UI, JSON e seleção)", "index.html#autotoc_md6", [
        [ "Solução de problemas (SDL2/SDL2_ttf)", "index.html#autotoc_md7", null ]
      ] ]
    ] ],
    [ "Compilar o firmware (RP2040)", "index.html#autotoc_md8", null ],
    [ "Persistência (host e RP2040)", "index.html#autotoc_md9", null ],
    [ "Parametrização (macros CFG_*)", "index.html#autotoc_md10", null ],
    [ "Documentação (Doxygen)", "index.html#autotoc_md11", null ],
    [ "Versão", "index.html#autotoc_md12", null ],
    [ "Contribuindo", "index.html#autotoc_md13", null ],
    [ "Logs e comandos (firmware)", "index.html#autotoc_md14", null ],
    [ "Licença", "index.html#autotoc_md15", null ],
    [ "RP2040 Maze Solver – Plano e Requisitos", "md_PLAN.html", [
      [ "Objetivo", "md_PLAN.html#autotoc_md17", null ],
      [ "Requisitos Funcionais", "md_PLAN.html#autotoc_md18", null ],
      [ "Requisitos Não Funcionais", "md_PLAN.html#autotoc_md19", null ],
      [ "Arquitetura / Pastas", "md_PLAN.html#autotoc_md20", null ],
      [ "Itens Implementados", "md_PLAN.html#autotoc_md21", [
        [ "Versão 0.0.1", "md_PLAN.html#autotoc_md22", null ],
        [ "Próximos passos versão 0.0.3", "md_PLAN.html#autotoc_md23", null ]
      ] ],
      [ "Próximos passos versão 0.0.4", "md_PLAN.html#autotoc_md24", null ],
      [ "Próximos passos (curto prazo)", "md_PLAN.html#autotoc_md25", null ],
      [ "Build Targets e Opções (CMake)", "md_PLAN.html#autotoc_md26", [
        [ "Observações do simulador (SDL2 e JSON)", "md_PLAN.html#autotoc_md27", null ]
      ] ],
      [ "Configuração por Macros (exemplos)", "md_PLAN.html#autotoc_md28", null ],
      [ "Métricas de Sucesso", "md_PLAN.html#autotoc_md29", null ],
      [ "Notas de Manutenção", "md_PLAN.html#autotoc_md30", null ],
      [ "Estado atual da documentação (Doxygen)", "md_PLAN.html#autotoc_md31", [
        [ "Convenções Doxygen", "md_PLAN.html#autotoc_md32", null ],
        [ "Definition of Done (DoD)", "md_PLAN.html#autotoc_md33", null ],
        [ "Próximas ações", "md_PLAN.html#autotoc_md34", null ]
      ] ]
    ] ],
    [ "Referências do Projeto", "md_REFERENCE.html", [
      [ "Simulador (controles e UI)", "md_REFERENCE.html#autotoc_md36", null ],
      [ "Formato JSON do labirinto", "md_REFERENCE.html#autotoc_md37", null ]
    ] ],
    [ "Estratégia de Navegação e Busca da Saída", "md_STRATEGIA.html", [
      [ "Visão Geral", "md_STRATEGIA.html#autotoc_md39", null ],
      [ "Regra da Mão Direita (Right-Hand)", "md_STRATEGIA.html#autotoc_md40", null ],
      [ "Observação de Paredes e Atualização do Mapa", "md_STRATEGIA.html#autotoc_md41", null ],
      [ "Planejamento (BFS)", "md_STRATEGIA.html#autotoc_md42", null ],
      [ "Decisão com Plano + Exploração", "md_STRATEGIA.html#autotoc_md43", null ],
      [ "Heurísticas e Reforço", "md_STRATEGIA.html#autotoc_md44", null ],
      [ "Conversão entre Direções Absolutas e Relativas", "md_STRATEGIA.html#autotoc_md45", null ],
      [ "Limitações Atuais", "md_STRATEGIA.html#autotoc_md46", null ],
      [ "Futuras Extensões", "md_STRATEGIA.html#autotoc_md47", null ],
      [ "Relacionado", "md_STRATEGIA.html#autotoc_md48", null ]
    ] ],
    [ "NAVIGATOR", "md_NAVIGATOR.html", [
      [ "Conceitos", "md_NAVIGATOR.html#autotoc_md50", null ],
      [ "Atualização do mapa a partir de sensores", "md_NAVIGATOR.html#autotoc_md51", null ],
      [ "Estratégias de decisão", "md_NAVIGATOR.html#autotoc_md52", null ],
      [ "Aprendizado por reforço simples (on-line)", "md_NAVIGATOR.html#autotoc_md53", null ],
      [ "Estados, início e objetivo", "md_NAVIGATOR.html#autotoc_md54", null ],
      [ "Interação com o simulador", "md_NAVIGATOR.html#autotoc_md55", null ],
      [ "Extensões sugeridas", "md_NAVIGATOR.html#autotoc_md56", null ]
    ] ],
    [ "SIMULATOR", "md_SIMULATOR.html", [
      [ "Visão geral", "md_SIMULATOR.html#autotoc_md58", null ],
      [ "Compilação e execução", "md_SIMULATOR.html#autotoc_md59", null ],
      [ "UI e Controles", "md_SIMULATOR.html#autotoc_md60", null ],
      [ "Cronômetro (tempo)", "md_SIMULATOR.html#autotoc_md61", null ],
      [ "Estados principais do episódio", "md_SIMULATOR.html#autotoc_md62", null ],
      [ "Persistência de labirinto (JSON)", "md_SIMULATOR.html#autotoc_md63", null ],
      [ "Exportação de solução (JSON versionado)", "md_SIMULATOR.html#autotoc_md64", null ],
      [ "Solução de problemas", "md_SIMULATOR.html#autotoc_md65", null ],
      [ "Limitações atuais", "md_SIMULATOR.html#autotoc_md66", null ],
      [ "Roadmap sugerido", "md_SIMULATOR.html#autotoc_md67", null ]
    ] ],
    [ "MotorControl: Como criar uma nova implementação (ex.: I2C)", "md_MotorControl__Implementations.html", [
      [ "Passos para criar uma nova implementação", "md_MotorControl__Implementations.html#autotoc_md69", null ],
      [ "Template mínimo (trechos)", "md_MotorControl__Implementations.html#autotoc_md70", null ]
    ] ],
    [ "Changelog", "md_CHANGELOG.html", [
      [ "<a href=\"https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/releases/tag/v0.0.2\" >0.0.2</a> - 2025-08-26", "md_CHANGELOG.html#autotoc_md72", [
        [ "Added", "md_CHANGELOG.html#autotoc_md73", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md74", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md75", null ],
        [ "Notes", "md_CHANGELOG.html#autotoc_md76", null ]
      ] ],
      [ "<a href=\"https://github.com/ArvoreDosSaberes/Maze_Solver_RP2040/releases/tag/v0.0.1\" >0.0.1</a> - 2025-08-25", "md_CHANGELOG.html#autotoc_md77", [
        [ "Added", "md_CHANGELOG.html#autotoc_md78", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md79", null ],
        [ "Notes", "md_CHANGELOG.html#autotoc_md80", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Enumerations", "functions_enum.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"IRSensorArray_8cpp.html",
"structStepLogEntry.html#adfee2d2a634a28fadf6bd4ba61dc11e5"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';