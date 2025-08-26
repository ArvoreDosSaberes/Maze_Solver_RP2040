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

## Formato JSON do labirinto

Arquivo salvo em `maze/*.json`. Estrutura típica:

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

Observação: veja comentários Doxygen nos headers em `src/core/` e `src/hal/` para detalhes de API.
