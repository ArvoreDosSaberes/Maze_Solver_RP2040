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
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include <cctype>
#include <cstdlib>
#include "core/MazeMap.hpp"
#include "core/Navigator.hpp"

using namespace maze;
namespace fs = std::filesystem;

/**
 * @brief Remove paredes entre duas células adjacentes (carvar passagem).
 */
static void carve_between(MazeMap& m, int x1, int y1, int x2, int y2) {
    if (x2 == x1 && y2 == y1-1) { m.set_wall(x1,y1,'N',false); }
    else if (x2 == x1+1 && y2 == y1) { m.set_wall(x1,y1,'E',false); }
    else if (x2 == x1 && y2 == y1+1) { m.set_wall(x1,y1,'S',false); }
    else if (x2 == x1-1 && y2 == y1) { m.set_wall(x1,y1,'W',false); }
}

// --- JSON e utilitários de filesystem ---
// Formato salvo:
// {
//   "width": W, "height": H,
//   "entrance": {"x":X, "y":Y, "heading":H},
//   "goal": {"x":X, "y":Y},
//   "cells": [ {"n":0/1, "e":0/1, "s":0/1, "w":0/1}, ... W*H ... ],
//   "meta": {"name":"...","email":"...","github":"...","date":"ISO"}
// }

static std::string iso_datetime_now() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    #if defined(_WIN32)
    localtime_s(&tm, &t);
    #else
    localtime_r(&t, &tm);
    #endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tm);
    return std::string(buf);
}

struct MetaInfo { std::string name, email, github, date; };

static MetaInfo collect_meta_default() {
    MetaInfo mi;
    const char* n = std::getenv("GIT_AUTHOR_NAME");
    const char* e = std::getenv("GIT_AUTHOR_EMAIL");
    const char* g = std::getenv("GITHUB_PROFILE");
    if (n) mi.name = n; if (e) mi.email = e; if (g) mi.github = g;
    if (mi.name.empty())  { std::printf("Digite seu nome (Enter para vazio): "); std::getline(std::cin, mi.name); }
    if (mi.email.empty()) { std::printf("Digite seu email (Enter para vazio): "); std::getline(std::cin, mi.email); }
    if (mi.github.empty()) { std::printf("Perfil GitHub (URL ou user, Enter para vazio): "); std::getline(std::cin, mi.github); }
    mi.date = iso_datetime_now();
    return mi;
}

static std::string escape_json(const std::string& s) {
    std::string o; o.reserve(s.size()+8);
    for (char c: s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default: o += c; break;
        }
    }
    return o;
}

static bool save_maze_json(const fs::path& file, const MazeMap& m, Point entrance, Point goal, uint8_t heading, const MetaInfo& meta) {
    std::ofstream ofs(file);
    if (!ofs) return false;
    ofs << "{\n";
    ofs << "  \"width\": " << m.width() << ", \"height\": " << m.height() << ",\n";
    ofs << "  \"entrance\": {\"x\": " << entrance.x << ", \"y\": " << entrance.y << ", \"heading\": " << (int)heading << "},\n";
    ofs << "  \"goal\": {\"x\": " << goal.x << ", \"y\": " << goal.y << "},\n";
    ofs << "  \"cells\": [\n";
    for (int y=0; y<m.height(); ++y) {
        for (int x=0; x<m.width(); ++x) {
            const Cell& c = m.at(x,y);
            ofs << "    {\"n\": " << (c.wall_n?1:0) << ", \"e\": " << (c.wall_e?1:0)
                << ", \"s\": " << (c.wall_s?1:0) << ", \"w\": " << (c.wall_w?1:0) << "}";
            if (!(x==m.width()-1 && y==m.height()-1)) ofs << ",";
            ofs << "\n";
        }
    }
    ofs << "  ],\n";
    ofs << "  \"meta\": {\n";
    ofs << "    \"name\": \"" << escape_json(meta.name) << "\",\n";
    ofs << "    \"email\": \"" << escape_json(meta.email) << "\",\n";
    ofs << "    \"github\": \"" << escape_json(meta.github) << "\",\n";
    ofs << "    \"date\": \"" << escape_json(meta.date) << "\"\n";
    ofs << "  }\n";
    ofs << "}\n";
    return true;
}

// Parser muito simples, assume JSON bem formado vindo do nosso save
static bool load_maze_json(const fs::path& file, MazeMap& m, Point& entrance, Point& goal, uint8_t& heading) {
    std::ifstream ifs(file);
    if (!ifs) return false;
    std::stringstream buffer; buffer << ifs.rdbuf();
    const std::string s = buffer.str();
    auto find_int = [&](const std::string& key, int def)->int{
        auto p = s.find("\""+key+"\""); if (p==std::string::npos) return def;
        p = s.find(':', p); if (p==std::string::npos) return def; ++p;
        while (p<s.size() && std::isspace((unsigned char)s[p])) ++p;
        int val=def; std::sscanf(s.c_str()+p, "%d", &val); return val;
    };
    int W = find_int("width", m.width());
    int H = find_int("height", m.height());
    if (W!=m.width() || H!=m.height()) {
        m = MazeMap(W,H);
    }
    auto find_obj_int = [&](const std::string& obj, const std::string& key, int def)->int{
        auto p = s.find("\""+obj+"\""); if (p==std::string::npos) return def;
        p = s.find('{' , p); if (p==std::string::npos) return def;
        auto q = s.find('}', p); if (q==std::string::npos) q = s.size();
        auto k = s.find("\""+key+"\"", p); if (k==std::string::npos || k>q) return def;
        k = s.find(':', k); if (k==std::string::npos) return def; ++k;
        while (k<s.size() && std::isspace((unsigned char)s[k])) ++k;
        int val=def; std::sscanf(s.c_str()+k, "%d", &val); return val;
    };
    entrance.x = find_obj_int("entrance", "x", 0);
    entrance.y = find_obj_int("entrance", "y", 0);
    heading    = (uint8_t)find_obj_int("entrance", "heading", 1);
    goal.x     = find_obj_int("goal", "x", W-1);
    goal.y     = find_obj_int("goal", "y", H-1);

    // Limpa paredes
    for (int y=0; y<m.height(); ++y) for (int x=0; x<m.width(); ++x) {
        m.set_wall(x,y,'N',false); m.set_wall(x,y,'E',false); m.set_wall(x,y,'S',false); m.set_wall(x,y,'W',false);
    }
    // Extrai array cells
    auto pcells = s.find("\"cells\""); if (pcells==std::string::npos) return true;
    pcells = s.find('[', pcells); if (pcells==std::string::npos) return true;
    size_t idx = 0; // idx em ordem linha-major
    for (size_t p = pcells; p < s.size() && idx < (size_t)(m.width()*m.height()); ) {
        p = s.find('{', p); if (p==std::string::npos) break; auto q = s.find('}', p); if (q==std::string::npos) break;
        auto sub = s.substr(p, q-p+1);
        int n=0,e=0,ss=0,w=0;
        std::sscanf(sub.c_str(), "{%*[^0-9]%d%*[^0-9]%d%*[^0-9]%d%*[^0-9]%d", &n,&e,&ss,&w);
        int x = idx % m.width(); int y = idx / m.width();
        if (n) m.set_wall(x,y,'N',true);
        if (e) m.set_wall(x,y,'E',true);
        if (ss) m.set_wall(x,y,'S',true);
        if (w) m.set_wall(x,y,'W',true);
        idx++;
        p = q+1;
    }
    return true;
}

static void ensure_dirs() {
    try {
        fs::create_directories("maze");
        fs::create_directories("make");
    } catch (...) { /* ignore */ }
}

static std::vector<fs::path> list_maze_files() {
    std::vector<fs::path> out;
    try {
        if (fs::exists("maze") && fs::is_directory("maze")) {
            for (auto& e : fs::directory_iterator("maze")) {
                if (e.is_regular_file()) {
                    auto p = e.path();
                    if (p.extension()==".json") out.push_back(p);
                }
            }
            std::sort(out.begin(), out.end());
        }
    } catch (...) {}
    return out;
}

/**
 * @brief Gera um labirinto perfeito via DFS aleatório, criando entrada e saída.
 *
 * - Inicializa todas as paredes como presentes.
 * - Executa DFS para carvar passagens.
 * - Cria uma entrada e uma saída em bordas opostas.
 *
 * @param m mapa a ser preenchido
 * @param W largura
 * @param H altura
 * @param entrance célula interna da entrada (retorno)
 * @param exit célula interna da saída (retorno)
 * @param entrance_heading orientação sugerida de início (0=N,1=E,2=S,3=W)
 */
static void generate_maze(MazeMap& m, int W, int H, Point& entrance, Point& goal_cell, uint8_t& entrance_heading) {
    // 1) Marca todas as paredes como presentes
    for (int y=0; y<H; ++y) {
        for (int x=0; x<W; ++x) {
            m.set_wall(x,y,'N',true);
            m.set_wall(x,y,'E',true);
            m.set_wall(x,y,'S',true);
            m.set_wall(x,y,'W',true);
        }
    }

    // 2) DFS iterativo para carvar
    std::random_device rd; std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dx(0, W-1), dy(0, H-1);
    int sx = dx(rng), sy = dy(rng);
    std::vector<uint8_t> vis(W*H, 0);
    auto idx = [&](int x,int y){ return y*W + x; };
    struct Node{int x,y;};
    std::vector<Node> stack;
    stack.push_back({sx,sy});
    vis[idx(sx,sy)] = 1;

    while (!stack.empty()) {
        auto [cx, cy] = stack.back();
        // vizinhos não visitados
        std::vector<Node> nbrs;
        if (cy>0 && !vis[idx(cx,cy-1)]) nbrs.push_back({cx,cy-1});
        if (cx<W-1 && !vis[idx(cx+1,cy)]) nbrs.push_back({cx+1,cy});
        if (cy<H-1 && !vis[idx(cx,cy+1)]) nbrs.push_back({cx,cy+1});
        if (cx>0 && !vis[idx(cx-1,cy)]) nbrs.push_back({cx-1,cy});
        if (!nbrs.empty()) {
            std::shuffle(nbrs.begin(), nbrs.end(), rng);
            auto [nx, ny] = nbrs.front();
            carve_between(m, cx, cy, nx, ny);
            vis[idx(nx,ny)] = 1;
            stack.push_back({nx,ny});
        } else {
            stack.pop_back();
        }
    }

    // 3) Escolhe entrada/saída em bordas opostas e abre a parede externa
    std::uniform_int_distribution<int> side_dist(0, 1);
    if (side_dist(rng) == 0) {
        // Entrada no oeste, saída no leste
        int ey = dy(rng);
        int oy = dy(rng);
        entrance = {0, ey};
        goal_cell     = {W-1, oy};
        m.set_wall(entrance.x, entrance.y, 'W', false); // abre para fora
        m.set_wall(goal_cell.x, goal_cell.y, 'E', false);
        entrance_heading = 1; // para Leste
    } else {
        // Entrada no norte, saída no sul
        int ex = dx(rng);
        int ox = dx(rng);
        entrance = {ex, 0};
        goal_cell     = {ox, H-1};
        m.set_wall(entrance.x, entrance.y, 'N', false);
        m.set_wall(goal_cell.x, goal_cell.y, 'S', false);
        entrance_heading = 2; // para Sul
    }
}

/**
 * @brief Desenha uma grade retangular para orientar a visualização do labirinto.
 *
 * Linhas horizontais e verticais espaçadas pelo tamanho da célula são traçadas
 * a partir de uma origem (ox, oy).
 *
 * @param ren Renderer SDL2 já inicializado.
 * @param ox Offset X (origem) em pixels.
 * @param oy Offset Y (origem) em pixels.
 * @param cell Tamanho da célula em pixels.
 * @param w Número de células no eixo X.
 * @param h Número de células no eixo Y.
 */
static void draw_grid(SDL_Renderer* ren, int ox, int oy, int cell, int w, int h) {
    SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    for (int y = 0; y <= h; ++y) {
        SDL_RenderDrawLine(ren, ox, oy + y*cell, ox + w*cell, oy + y*cell);
    }
    for (int x = 0; x <= w; ++x) {
        SDL_RenderDrawLine(ren, ox + x*cell, oy, ox + x*cell, oy + h*cell);
    }
}

/**
 * @brief Gera uma leitura de sensores relativa (esquerda, frente, direita) a partir do mapa.
 *
 * Converte paredes absolutas da célula atual em flags de caminho livre relativo ao
 * heading do agente.
 *
 * @param m Referência para o `maze::MazeMap` (mapa estático conhecido pelo simulador).
 * @param cell Posição da célula atual do agente.
 * @param heading Direção absoluta do agente (0=N, 1=E, 2=S, 3=W).
 * @return Estrutura `maze::SensorRead` com as flags `left_free`, `front_free`, `right_free`.
 */
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

/**
 * @brief Verifica se existe passagem livre na direção absoluta informada.
 *
 * @param m Mapa do labirinto.
 * @param cell Célula atual.
 * @param absdir Direção absoluta desejada ('N', 'E', 'S', 'W').
 * @return true se não houver parede nessa direção; false caso contrário.
 */
static bool can_move(const MazeMap& m, Point cell, char absdir) {
    const Cell& c = m.at(cell.x, cell.y);
    if (absdir=='N') return !c.wall_n;
    if (absdir=='E') return !c.wall_e;
    if (absdir=='S') return !c.wall_s;
    if (absdir=='W') return !c.wall_w;
    return false;
}

/**
 * @brief Aplica a ação do agente atualizando orientação e/ou posição.
 *
 * @param cell Posição do agente (atualizada em caso de avanço).
 * @param heading Direção absoluta (0=N,1=E,2=S,3=W), atualizada para giros.
 * @param a Ação decidida pelo `Navigator` (`Left`, `Right`, `Back`, `Forward`).
 */
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

/**
 * @brief Desenha as paredes do labirinto conforme o conteúdo de `MazeMap`.
 *
 * @param ren Renderer SDL2.
 * @param m Referência ao mapa do labirinto.
 * @param ox Offset X em pixels da origem do desenho.
 * @param oy Offset Y em pixels da origem do desenho.
 * @param cell Tamanho da célula em pixels.
 * @param thick Espessura do traço de parede (pixels). Default: 3.
 */
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

/**
 * @brief Desenha o agente (corpo e indicação de orientação) na célula informada.
 *
 * @param ren Renderer SDL2.
 * @param p Posição da célula do agente.
 * @param heading Direção absoluta (0=N,1=E,2=S,3=W).
 * @param ox Offset X em pixels.
 * @param oy Offset Y em pixels.
 * @param cell Tamanho da célula em pixels.
 */
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

/**
 * @brief Ponto de entrada do simulador 2D com SDL2.
 *
 * Inicializa SDL2, constrói um mapa de exemplo, configura o `maze::Navigator` e
 * executa o loop principal com renderização e controle por teclado.
 *
 * Parâmetros de linha de comando não são utilizados neste exemplo.
 *
 * @param argc Quantidade de argumentos.
 * @param argv Vetor de argumentos.
 * @return 0 em término normal; 1 se ocorrer erro de inicialização SDL.
 */
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

    const int CELL = 40;
    const int OX = 50, OY = 50;
    ensure_dirs();

    // Menu de seleção (GUI mínima via título da janela)
    // Itens: [0] Aleatório ... depois arquivos encontrados em maze/
    auto files = list_maze_files();
    std::vector<std::string> items;
    items.push_back("Aleatório (gerar e salvar em make/)");
    for (auto& p: files) items.push_back(p.filename().string());
    int sel = 0;

    bool choosing = true;
    int W = 16, H = 12; // padrão; pode ser sobrescrito por arquivo
    MazeMap map(W, H);
    Point entrance{}, goal_cell{};
    uint8_t entrance_heading = 1;

    SDL_SetWindowTitle(win, ("Escolha: " + items[sel]).c_str());
    while (choosing) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { choosing = false; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) { choosing = false; }
                else if (e.key.keysym.sym == SDLK_UP) { sel = (sel + (int)items.size() - 1) % (int)items.size(); SDL_SetWindowTitle(win, ("Escolha: " + items[sel]).c_str()); }
                else if (e.key.keysym.sym == SDLK_DOWN) { sel = (sel + 1) % (int)items.size(); SDL_SetWindowTitle(win, ("Escolha: " + items[sel]).c_str()); }
                else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                    // Executa seleção
                    if (sel == 0) {
                        // Aleatório
                        generate_maze(map, W, H, entrance, goal_cell, entrance_heading);
                        // Salva
                        MetaInfo mi = collect_meta_default();
                        char fname[128];
                        std::snprintf(fname, sizeof(fname), "maze_%dx%d_%ld.json", W, H, (long)std::time(nullptr));
                        fs::path out = fs::path("make") / fname;
                        if (!save_maze_json(out, map, entrance, goal_cell, entrance_heading, mi)) {
                            std::fprintf(stderr, "Falha ao salvar %s\n", out.string().c_str());
                        } else {
                            std::printf("Salvo: %s\n", out.string().c_str());
                        }
                    } else {
                        // Carrega arquivo
                        fs::path f = files[sel-1];
                        // Inicia com dimensões padrão; loader pode redimensionar
                        if (!load_maze_json(f, map, entrance, goal_cell, entrance_heading)) {
                            std::fprintf(stderr, "Falha ao carregar %s, gerando aleatório.\n", f.string().c_str());
                            generate_maze(map, W, H, entrance, goal_cell, entrance_heading);
                        } else {
                            W = map.width(); H = map.height();
                        }
                    }
                    choosing = false;
                }
            }
        }
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        // Desenha apenas grade para indicar que está em menu
        draw_grid(ren, OX, OY, CELL, 8, 4);
        SDL_RenderPresent(ren);
    }
    if (SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN) {
        SDL_SetWindowTitle(win, "Maze Simulator");
    }

    // Navigator e planejamento do caminho da entrada até a saída
    maze::Navigator nav;
    nav.setStrategy(maze::Navigator::Strategy::RightHand);
    nav.setMapDimensions(W, H);
    Point start = entrance;
    Point goal = goal_cell;
    nav.setStartGoal(start, goal);
    // Copia o mapa real gerado para o mapa interno do Navigator (para o BFS conhecer as paredes reais)
    nav.map() = map;
    Point agent = start;
    uint8_t heading = entrance_heading;
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
                    agent = start; heading = entrance_heading; steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                    nav.setMapDimensions(W, H);
                    nav.setStartGoal(start, goal);
                    nav.map() = map;
                    nav.planRoute();
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
            // debug: imprime decisão
            std::printf("pos=(%d,%d) head=%u act=%d free[L=%d F=%d R=%d]\n", agent.x, agent.y, heading, (int)dec.action, (int)sr.left_free, (int)sr.front_free, (int)sr.right_free);
            // Check if action would hit a wall when moving forward
            bool moved = false;
            if (dec.action == maze::Action::Forward) {
                const char abs_dirs[4] = {'N','E','S','W'};
                char absdir = abs_dirs[heading];
                if (can_move(map, agent, absdir)) {
                    apply_move(agent, heading, dec.action);
                    moved = true;
                } else {
                    // Plano inválido localmente: replaneja e faz fallback heurístico neste passo
                    collisions++;
                    nav.planRoute();
                    auto dec2 = nav.decide(sr);
                    if (dec2.action == maze::Action::Forward) {
                        absdir = abs_dirs[heading];
                        if (can_move(map, agent, absdir)) { apply_move(agent, heading, dec2.action); moved = true; }
                    } else {
                        apply_move(agent, heading, dec2.action); moved = true;
                    }
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
