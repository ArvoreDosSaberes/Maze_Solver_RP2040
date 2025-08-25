/**
 * @file simulator/main.cpp
 * @brief Simulador SDL2 do resolvedor de labirintos (visualização 2D).
 *
 * Permite visualizar o agente (vermelho) navegando em um labirinto cujas
 * paredes são desenhadas em verde. O simulador usa `maze::Navigator` e um
 * mapa `maze::MazeMap` com um exemplo simples.
 *
 * Como executar:
 * - Habilite o alvo do simulador no CMake: `-DBUILD_SIM=ON`.
 * - Garanta a dependência da SDL2 instalada no sistema (dev headers).
 * - Rode o executável gerado (ex.: `./simulator_app`).
 *
 * Controles:
 * - ESC: sair
 * - Espaço: pausar/continuar
 * - R: resetar agente/tempo
 *
 * @since 0.1
 */
#include <SDL2/SDL.h>
#include <cstdio>
#include "core/MazeMap.hpp"
#include "core/Navigator.hpp"

using namespace maze;

static void draw_grid(SDL_Renderer* ren, int ox, int oy, int cell, int w, int h) {
    SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    for (int y = 0; y <= h; ++y) {
        SDL_RenderDrawLine(ren, ox, oy + y*cell, ox + w*cell, oy + y*cell);
    }
    for (int x = 0; x <= w; ++x) {
        SDL_RenderDrawLine(ren, ox + x*cell, oy, ox + x*cell, oy + h*cell);
    }
}

static maze::SensorRead make_sensor_read(const MazeMap& m, Point cell, uint8_t heading) {
    // Convert absolute walls into relative free flags
    maze::SensorRead sr{};
    const Cell& c = m.at(cell.x, cell.y);
    auto abs_left  = (heading + 3) & 3;
    auto abs_front = heading;
    auto abs_right = (heading + 1) & 3;
    // abs index 0=N,1=E,2=S,3=W
    auto is_free = [&](int absdir){
        switch (absdir) {
            case 0: return !c.wall_n; case 1: return !c.wall_e; case 2: return !c.wall_s; default: return !c.wall_w;
        }
    };
    sr.left_free  = is_free(abs_left);
    sr.front_free = is_free(abs_front);
    sr.right_free = is_free(abs_right);
    return sr;
}

static bool can_move(const MazeMap& m, Point cell, char absdir) {
    const Cell& c = m.at(cell.x, cell.y);
    if (absdir=='N') return !c.wall_n;
    if (absdir=='E') return !c.wall_e;
    if (absdir=='S') return !c.wall_s;
    if (absdir=='W') return !c.wall_w;
    return false;
}

static void apply_move(Point& cell, uint8_t& heading, maze::Action a) {
    // Update heading for Left/Right/Back, and position for Forward
    if (a == maze::Action::Left)  heading = (heading + 3) & 3;
    else if (a == maze::Action::Right) heading = (heading + 1) & 3;
    else if (a == maze::Action::Back) heading = (heading + 2) & 3;
    else {
        // forward: move based on heading
        if (heading==0) cell.y -= 1;
        else if (heading==1) cell.x += 1;
        else if (heading==2) cell.y += 1;
        else cell.x -= 1;
    }
}

static void draw_maze(SDL_Renderer* ren, const MazeMap& m, int ox, int oy, int cell, int thick=3) {
    SDL_SetRenderDrawColor(ren, 0, 200, 0, 255);
    for (int y = 0; y < m.height(); ++y) {
        for (int x = 0; x < m.width(); ++x) {
            const Cell& c = m.at(x,y);
            const int x0 = ox + x*cell;
            const int y0 = oy + y*cell;
            if (c.wall_n) { SDL_Rect r{ x0, y0 - thick/2, cell, thick }; SDL_RenderFillRect(ren, &r); }
            if (c.wall_s) { SDL_Rect r{ x0, y0 + cell - thick/2, cell, thick }; SDL_RenderFillRect(ren, &r); }
            if (c.wall_w) { SDL_Rect r{ x0 - thick/2, y0, thick, cell }; SDL_RenderFillRect(ren, &r); }
            if (c.wall_e) { SDL_Rect r{ x0 + cell - thick/2, y0, thick, cell }; SDL_RenderFillRect(ren, &r); }
        }
    }
}

static void draw_agent(SDL_Renderer* ren, Point p, int heading, int ox, int oy, int cell) {
    // body
    SDL_SetRenderDrawColor(ren, 200, 0, 0, 255);
    SDL_Rect body{ ox + p.x*cell + cell/4, oy + p.y*cell + cell/4, cell/2, cell/2 };
    SDL_RenderFillRect(ren, &body);
    // heading as a small line
    SDL_SetRenderDrawColor(ren, 255, 180, 180, 255);
    int cx = ox + p.x*cell + cell/2;
    int cy = oy + p.y*cell + cell/2;
    int hx = cx, hy = cy;
    const int d = cell/3;
    if (heading == 0) { hy -= d; }
    else if (heading == 1) { hx += d; }
    else if (heading == 2) { hy += d; }
    else { hx -= d; }
    SDL_RenderDrawLine(ren, cx, cy, hx, hy);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    const int win_w = 800, win_h = 700;
    SDL_Window* win = SDL_CreateWindow("Maze Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, SDL_WINDOW_SHOWN);
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        std::fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    const int W = 8, H = 8, CELL = 60;
    const int OX = 50, OY = 50;
    MazeMap map(W, H);
    // exemplo de paredes (um quadrado interno)
    for (int x=2; x<6; ++x) { map.set_wall(x,2,'N',true); map.set_wall(x,5,'S',true); }
    for (int y=2; y<6; ++y) { map.set_wall(2,y,'W',true); map.set_wall(5,y,'E',true); }

    // Navigator setup with planning from start to goal
    maze::Navigator nav;
    nav.setStrategy(maze::Navigator::Strategy::RightHand);
    nav.setMapDimensions(W, H);
    Point start{0,0};
    Point goal{W-1,H-1};
    nav.setStartGoal(start, goal);
    // Observe boundary walls from the static map for initial cell (optional in this simple demo)
    Point agent{0,0};
    uint8_t heading = 1; // East
    nav.planRoute();

    Uint32 start_ms = SDL_GetTicks();
    Uint32 last_step = start_ms;
    int steps = 0;
    int collisions = 0;
    bool running = true;
    bool paused = false;
    while (running) {
        SDL_Event e; 
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_SPACE) paused = !paused;
                if (e.key.keysym.sym == SDLK_r) {
                    agent = start; heading = 1; steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                }
            }
        }

        // avanço baseado no Navigator e plano (fallback RightHand)
        Uint32 now = SDL_GetTicks();
        if (!paused && (now - last_step > 250)) {
            last_step = now;
            maze::SensorRead sr = make_sensor_read(map, agent, heading);
            // opcional: atualizar conhecimento do mapa
            nav.observeCellWalls(agent, sr, heading);
            auto dec = nav.decidePlanned(agent, heading, sr);
            // Check if action would hit a wall when moving forward
            bool moved = false;
            if (dec.action == maze::Action::Forward) {
                const char abs_dirs[4] = {'N','E','S','W'};
                char absdir = abs_dirs[heading];
                if (can_move(map, agent, absdir)) {
                    apply_move(agent, heading, dec.action);
                    moved = true;
                } else {
                    collisions++;
                }
            } else {
                apply_move(agent, heading, dec.action);
                moved = true;
            }
            if (moved) steps++;
            if (agent.x==goal.x && agent.y==goal.y) {
                float sim_time_s = (SDL_GetTicks() - start_ms) / 1000.0f;
                int cost = steps + collisions * 5;
                std::printf("Reached goal in %d steps, collisions=%d, time=%.2fs, cost=%d\n", steps, collisions, sim_time_s, cost);
                paused = true;
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        draw_grid(ren, OX, OY, CELL, W, H);
        draw_maze(ren, map, OX, OY, CELL);
        draw_agent(ren, agent, heading, OX, OY, CELL);
        float sim_time_s = (SDL_GetTicks() - start_ms) / 1000.0f;
        int cost = steps + collisions * 5;
        char title[160];
        std::snprintf(title, sizeof(title), "Maze Simulator - steps=%d col=%d time=%.1fs cost=%d %s", steps, collisions, sim_time_s, cost, paused?"(paused)":"");
        SDL_SetWindowTitle(win, title);
        SDL_RenderPresent(ren);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
