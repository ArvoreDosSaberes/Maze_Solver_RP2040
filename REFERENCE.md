# Referências do Projeto

- Micromouse e navegação em labirintos
  - IEEE Micromouse Resources: https://ctms.engin.umich.edu/wp-content/uploads/sites/441/2019/06/Micromouse.pdf
  - Micromouse Online (conceitos de mapas, flood fill): https://www.micromouseonline.com/
- Algoritmos de caminho
  - Dijkstra e A*: "Introduction to Algorithms" (Cormen et al.)
  - Flood fill em grids: https://en.wikipedia.org/wiki/Flood_fill
- Controle de movimento/motores DC
  - PID básico: https://tttapa.github.io/Pages/Arduino/Control/Projects/Project%20Servo%20Motor/Control-Theory.html
  - Ponte H e PWM: documentos de aplicação de fabricantes (ex. TI DRV8x, Infineon BTN8982)
- Sensores IR refletivos
  - Princípios de leitura analógica e filtragem EMA
- RP2040 / Pico SDK
  - Raspberry Pi Pico C/C++ SDK: https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
  - Pico SDK examples: https://github.com/raspberrypi/pico-examples
- Unity Test Framework (C unit testing)
  - https://github.com/ThrowTheSwitch/Unity
- SDL2 (simulador)
  - https://wiki.libsdl.org/SDL2/FrontPage
- CMake
  - Modern CMake: https://cliutils.gitlab.io/modern-cmake/

## Simulador (controles e UI)

- Controles: setas ou WASD para navegar no menu/seleção e iniciar.
- UI:
  - Sidebar (lateral direita): log de eventos e métricas (passos, colisões, custo).
  - Botões: "Iniciar/Parar" e "Novo" (gera labirinto aleatório, salva e reinicia).
  - Modal de metadados: exibido uma vez por sessão quando `SDL2_ttf` está disponível.
- Dependência de texto: rótulos/modal requerem `SDL2_ttf`; sem ele, o simulador roda sem textos.

## Formatos de arquivo (.maze/.soluct/.plan)

Arquivos salvos em `maze/` com as seguintes extensões:
- `.maze`: mapa do labirinto (conteúdo JSON)
- `.soluct`: solução final (conteúdo JSON)
- `.plan`: plano/tentativa por passo (conteúdo JSON)

Estrutura típica de um `.maze`:

```json
{
  "width": 16,
  "height": 16,
  "entrance": [0, 0],
  "goal": [15, 15],
  "walls": [
    // representação compacta por células ou arestas conforme implementação
  ],
  "meta": {
    "name": "Seu Nome",
    "email": "seu@email",
    "github": "https://github.com/usuario",
    "date": "2025-08-26T12:34:56Z"
  }
}
```

- Metadados (bloco `meta`):
  - Origem: variáveis de ambiente `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GITHUB_PROFILE`.
  - Interativo: quando `SDL2_ttf` presente, um modal solicita/permite editar os campos.
  - Sem `SDL2_ttf`: usa valores padrão (env vars) e timestamp ISO 8601.

Estrutura resumida de um `.soluct` (versão N):

```json
{
  "map_file": "maze/maze_16x12_1756227259.maze",
  "width": 16,
  "height": 12,
  "entrance": [0, 0, 1],
  "goal": [15, 11],
  "metrics": { "steps": 123, "collisions": 2, "time_s": 4.21, "cost": 121.0 },
  "path": [[0,0],[1,0], [2,0] /* ... */],
  "meta": { "name": "...", "email": "...", "github": "...", "date": "..." }
}
```

Estrutura resumida de um `.plan` (tentativa M):

```json
{
  "map_file": "maze/maze_16x12_1756227259.maze",
  "width": 16,
  "height": 12,
  "entrance": [0, 0, 1],
  "goal": [15, 11],
  "steps": [
    {
      "from": [0,0],
      "to": [1,0],
      "heading_before": 1,
      "action": "forward",
      "moved": true,
      "event": "forward",
      "collisions": 0,
      "delta_score": 1.0,
      "score_after": 1.0,
      "step_index": 0
    }
    /* ... */
  ],
  "summary": { "result": "success", "steps": 123, "collisions": 2, "score": 121.0 },
  "meta": { "name": "...", "email": "...", "github": "...", "date": "..." }
}
```

Observação: veja comentários Doxygen nos headers em `src/core/` e `src/hal/` para detalhes de API.
