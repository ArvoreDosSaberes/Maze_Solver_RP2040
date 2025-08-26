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
#ifdef HAVE_SDL_TTF
#include <SDL2/SDL_ttf.h>
#endif
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

// --- UI helpers (font handle) defined early so it can be referenced below ---
struct UIFont {
#ifdef HAVE_SDL_TTF
    TTF_Font* font{nullptr};
#else
    void* font{nullptr};
#endif
    bool ok{false};
};

// Forward declaration for text drawing used by input helpers
static void draw_text(SDL_Renderer* ren, UIFont& f, const std::string& text, int x, int y, SDL_Color color);

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

static MetaInfo collect_meta_default_noninteractive() {
    MetaInfo mi;
    const char* n = std::getenv("GIT_AUTHOR_NAME");
    const char* e = std::getenv("GIT_AUTHOR_EMAIL");
    const char* g = std::getenv("GITHUB_PROFILE");
    if (n) mi.name = n; if (e) mi.email = e; if (g) mi.github = g;
    mi.date = iso_datetime_now();
    return mi;
}

// Session metadata state
static bool g_meta_set = false;
static MetaInfo g_session_meta;

// Preferred metadata for saving new mazes
static MetaInfo collect_meta_default() {
#ifdef HAVE_SDL_TTF
    if (g_meta_set) return g_session_meta;
#endif
    return collect_meta_default_noninteractive();
}

#ifdef HAVE_SDL_TTF
// simple GUI modal to capture text into provided string
static void input_field(SDL_Renderer* ren, UIFont& font, const SDL_Rect& r, const char* label, std::string& value, bool focused) {
    // box
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 180, 180, 200, 255);
    SDL_RenderDrawRect(ren, &r);
    // label
    draw_text(ren, font, label, r.x, r.y - 18, SDL_Color{200,200,220,255});
    // text
    draw_text(ren, font, value.empty()? std::string(""): value, r.x + 6, r.y + 6, SDL_Color{230,230,255,255});
    if (focused) {
        // caret
        int cx = r.x + 6 + (int)value.size()*8; // rough estimate
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderDrawLine(ren, cx, r.y + 4, cx, r.y + r.h - 4);
    }
}

static void ensure_session_meta(SDL_Renderer* ren, UIFont& font, int win_w, int win_h) {
    if (g_meta_set) return;
    g_session_meta = collect_meta_default_noninteractive();
    SDL_StartTextInput();
    bool in_modal = true;
    int focus = 0; // 0=name,1=email,2=github
    SDL_Rect modal{ win_w/2 - 220, win_h/2 - 140, 440, 260 };
    SDL_Rect r_name{ modal.x + 20, modal.y + 70, modal.w - 40, 30 };
    SDL_Rect r_mail{ modal.x + 20, modal.y + 120, modal.w - 40, 30 };
    SDL_Rect r_gh  { modal.x + 20, modal.y + 170, modal.w - 40, 30 };
    SDL_Rect btn_ok{ modal.x + modal.w - 110, modal.y + modal.h - 44, 90, 28 };
    SDL_Rect btn_skip{ modal.x + 20, modal.y + modal.h - 44, 90, 28 };
    auto draw_modal = [&](){
        // dim bg
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 160);
        SDL_Rect full{0,0,win_w,win_h};
        SDL_RenderFillRect(ren, &full);
        // panel
        SDL_SetRenderDrawColor(ren, 25, 25, 35, 255);
        SDL_RenderFillRect(ren, &modal);
        SDL_SetRenderDrawColor(ren, 200, 200, 220, 255);
        SDL_RenderDrawRect(ren, &modal);
        draw_text(ren, font, "Informações do usuário (uma vez por sessão)", modal.x + 20, modal.y + 20, SDL_Color{220,220,240,255});
        input_field(ren, font, r_name,  "Nome",   g_session_meta.name,  focus==0);
        input_field(ren, font, r_mail,  "Email",  g_session_meta.email, focus==1);
        input_field(ren, font, r_gh,    "GitHub", g_session_meta.github,focus==2);
        // buttons
        SDL_SetRenderDrawColor(ren, 60, 60, 90, 255);
        SDL_RenderFillRect(ren, &btn_ok);
        SDL_RenderFillRect(ren, &btn_skip);
        SDL_SetRenderDrawColor(ren, 160, 160, 200, 255);
        SDL_RenderDrawRect(ren, &btn_ok);
        SDL_RenderDrawRect(ren, &btn_skip);
        draw_text(ren, font, "Salvar", btn_ok.x + 14, btn_ok.y + 6, SDL_Color{230,230,255,255});
        draw_text(ren, font, "Pular",  btn_skip.x + 14, btn_skip.y + 6, SDL_Color{230,230,255,255});
    };
    while (in_modal) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { in_modal = false; break; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_TAB) { focus = (focus + 1) % 3; }
                else if (e.key.keysym.sym == SDLK_RETURN) { in_modal = false; }
                else if (e.key.keysym.sym == SDLK_ESCAPE) { in_modal = false; }
                else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    std::string* t = (focus==0? &g_session_meta.name : (focus==1? &g_session_meta.email : &g_session_meta.github));
                    if (!t->empty()) t->pop_back();
                }
            } else if (e.type == SDL_TEXTINPUT) {
                std::string* t = (focus==0? &g_session_meta.name : (focus==1? &g_session_meta.email : &g_session_meta.github));
                (*t) += e.text.text;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                auto inside = [&](SDL_Rect r){ return mx>=r.x && mx<r.x+r.w && my>=r.y && my<r.y+r.h; };
                if (inside(r_name)) focus = 0; else if (inside(r_mail)) focus = 1; else if (inside(r_gh)) focus = 2;
                else if (inside(btn_ok)) in_modal = false;
                else if (inside(btn_skip)) { g_session_meta = collect_meta_default_noninteractive(); in_modal = false; }
            }
        }
        // redraw underlay is caller's responsibility; just draw modal layer
        draw_modal();
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    SDL_StopTextInput();
    g_session_meta.date = iso_datetime_now();
    g_meta_set = true;
}
#else
static void ensure_session_meta(SDL_Renderer* /*ren*/, UIFont& /*font*/, int /*win_w*/, int /*win_h*/) {
    if (g_meta_set) return;
    g_session_meta = collect_meta_default_noninteractive();
    g_meta_set = true;
}
#endif

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

// --- UI helpers (optional text rendering via SDL_ttf) ---
// UIFont is defined earlier near the top of this file.

static UIFont ui_font_init(int pt=14) {
    UIFont f;
#ifdef HAVE_SDL_TTF
    if (TTF_WasInit()==0) {
        if (TTF_Init()!=0) return f;
    }
    // Try common system font
    const char* candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        nullptr
    };
    for (int i=0; candidates[i]; ++i) {
        f.font = TTF_OpenFont(candidates[i], pt);
        if (f.font) { f.ok = true; break; }
    }
#else
    (void)pt;
#endif
    return f;
}

static void ui_font_destroy(UIFont& f) {
#ifdef HAVE_SDL_TTF
    if (f.font) { TTF_CloseFont((TTF_Font*)f.font); f.font=nullptr; }
#else
    (void)f;
#endif
}

static void draw_text(SDL_Renderer* ren, UIFont& f, const std::string& text, int x, int y, SDL_Color color) {
#ifdef HAVE_SDL_TTF
    if (!f.ok || !f.font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended((TTF_Font*)f.font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst{ x, y, surf->w, surf->h };
    SDL_RenderCopy(ren, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
#else
    (void)ren; (void)f; (void)text; (void)x; (void)y; (void)color;
#endif
}

struct UIButton {
    SDL_Rect rect{0,0,0,0};
    bool enabled{true};
    std::string label;
};

static void draw_button(SDL_Renderer* ren, UIFont& f, const UIButton& b) {
    // background
    if (b.enabled) SDL_SetRenderDrawColor(ren, 60, 60, 90, 255);
    else SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    SDL_RenderFillRect(ren, &b.rect);
    // border
    SDL_SetRenderDrawColor(ren, 160, 160, 200, 255);
    SDL_RenderDrawRect(ren, &b.rect);
    // text
    SDL_Color c = b.enabled ? SDL_Color{230,230,255,255} : SDL_Color{120,120,140,255};
    draw_text(ren, f, b.label, b.rect.x + 8, b.rect.y + 6, c);
}

// simple log buffer
struct LogLine { std::string text; SDL_Color color; };

static void draw_sidebar(SDL_Renderer* ren, UIFont& f, const SDL_Rect& sidebar, const std::vector<LogLine>& log, int max_lines_draw) {
    // background
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
    SDL_RenderFillRect(ren, &sidebar);
    // title
    draw_text(ren, f, "Eventos", sidebar.x + 10, sidebar.y + 10, SDL_Color{200,200,220,255});
    int y = sidebar.y + 30;
    int drawn = 0;
    int start = (int)log.size() - max_lines_draw;
    if (start < 0) start = 0;
    for (size_t i = start; i < log.size(); ++i) {
        draw_text(ren, f, log[i].text, sidebar.x + 10, y, log[i].color);
        y += 18;
        if (++drawn >= max_lines_draw) break;
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
    const int win_w = 1000, win_h = 700; // wider for sidebar
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

    // Optional font (graceful if missing)
    UIFont font = ui_font_init(14);

    const int sidebar_w = 260;
    const int CELL = 40;
    const int OX = 50, OY = 50;
    SDL_Rect sidebar{ win_w - sidebar_w, 0, sidebar_w, win_h };
    ensure_dirs();

    // Menu de seleção (GUI mínima via título da janela)
    // Itens: [0] Aleatório ... depois arquivos encontrados em maze/
    auto files = list_maze_files();
    std::vector<std::string> items;
    if (files.empty()) {
        // Se não houver labirintos pré-gravados, solicite identificação do usuário antes de qualquer salvamento futuro
        ensure_session_meta(ren, font, win_w, win_h);
    }
    items.push_back("Aleatório (gerar e salvar em maze/)");
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
                        ensure_session_meta(ren, font, win_w, win_h);
                        MetaInfo mi = collect_meta_default();
                        char fname[128];
                        std::snprintf(fname, sizeof(fname), "maze_%dx%d_%ld.json", W, H, (long)std::time(nullptr));
                        fs::path out = fs::path("maze") / fname;
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
        // Área esquerda: grade sutil de fundo
        draw_grid(ren, OX, OY, CELL, 8, 4);
        // Painel de seleção
        SDL_Rect panel{ OX, OY, 520, 360 };
        SDL_SetRenderDrawColor(ren, 25, 25, 35, 255);
        SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 160, 160, 200, 255);
        SDL_RenderDrawRect(ren, &panel);
        draw_text(ren, font, "Selecione um labirinto (Enter)", panel.x + 12, panel.y + 10, SDL_Color{210,210,230,255});
        int y = panel.y + 40;
        for (size_t i = 0; i < items.size(); ++i) {
            // destaque da seleção
            if ((int)i == sel) {
                SDL_Rect hl{ panel.x + 8, y - 4, panel.w - 16, 24 };
                SDL_SetRenderDrawColor(ren, 50, 50, 90, 255);
                SDL_RenderFillRect(ren, &hl);
            }
            draw_text(ren, font, items[i], panel.x + 16, y, SDL_Color{230,230,255,255});
            y += 24;
            if (y > panel.y + panel.h - 24) break; // evita overflow visual
        }
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
    Uint32 frozen_ms = 0;
    bool time_frozen = false;
    Uint32 last_step = start_ms;
    int steps = 0;
    int collisions = 0;
    bool running = true;
    bool paused = false;

    // UI state machine
    enum class Phase { Ready, RunningExplore, RunningReplay, FinishedSuccess, FinishedFail };
    Phase phase = Phase::Ready; // after maze loaded/generated
    const int max_steps_fail = W * H * 8;

    // Buttons
    UIButton btnStart{ SDL_Rect{ sidebar.x + 20, 60, sidebar_w - 40, 34 }, true, "Iniciar" };
    UIButton btnNew{ SDL_Rect{ sidebar.x + 20, 100, sidebar_w - 40, 34 }, true, "Novo Labirinto" };

    // Log buffer
    std::vector<LogLine> log;
    auto push_log = [&](const std::string& s, SDL_Color c){ log.push_back({s,c}); if (log.size() > 1000) log.erase(log.begin(), log.begin()+500); };
    push_log("Pronto. Selecione Iniciar.", SDL_Color{180,220,180,255});
    while (running) {
        SDL_Event e; 
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_SPACE) paused = !paused;
                if (e.key.keysym.sym == SDLK_r) {
                    agent = start; heading = entrance_heading; steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                    start_ms = last_step; time_frozen = false; frozen_ms = 0;
                    nav.setMapDimensions(W, H);
                    nav.setStartGoal(start, goal);
                    nav.map() = map;
                    nav.planRoute();
                    phase = Phase::Ready;
                    btnStart.label = "Iniciar"; btnStart.enabled = true;
                    // btnNew remains enabled at all times after a maze is loaded
                    log.clear(); push_log("Resetado.", SDL_Color{200,200,200,255});
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                auto in_rect = [&](const SDL_Rect& r){ return mx>=r.x && mx<r.x+r.w && my>=r.y && my<r.y+r.h; };
                if (btnStart.enabled && in_rect(btnStart.rect)) {
                    if (phase == Phase::Ready || phase == Phase::FinishedSuccess) {
                        // Start exploration or replay learned path
                        agent = start; heading = entrance_heading; steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                        start_ms = last_step; time_frozen = false; frozen_ms = 0;
                        nav.setMapDimensions(W, H);
                        nav.setStartGoal(start, goal);
                        nav.map() = map;
                        nav.planRoute();
                        phase = (phase==Phase::FinishedSuccess) ? Phase::RunningReplay : Phase::RunningExplore;
                        btnStart.label = "Parar";
                        push_log("Execução iniciada.", SDL_Color{180,220,180,255});
                    } else if (phase == Phase::RunningExplore || phase == Phase::RunningReplay) {
                        paused = true; phase = Phase::Ready; btnStart.label = "Iniciar"; push_log("Execução parada.", SDL_Color{220,180,180,255});
                    } else if (phase == Phase::FinishedFail) {
                        // Test again
                        agent = start; heading = entrance_heading; steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                        start_ms = last_step; time_frozen = false; frozen_ms = 0;
                        nav.setMapDimensions(W, H);
                        nav.setStartGoal(start, goal);
                        nav.map() = map;
                        nav.planRoute();
                        phase = Phase::RunningExplore; btnStart.label = "Parar"; push_log("Teste reiniciado.", SDL_Color{180,220,180,255});
                    }
                }
                if (btnNew.enabled && in_rect(btnNew.rect)) {
                    // Generate a new random maze and save; then reset to Ready
                    generate_maze(map, W, H, entrance, goal_cell, entrance_heading);
                    ensure_session_meta(ren, font, win_w, win_h);
                    MetaInfo mi = collect_meta_default();
                    char fname[128];
                    std::snprintf(fname, sizeof(fname), "maze_%dx%d_%ld.json", W, H, (long)std::time(nullptr));
                    fs::path out = fs::path("maze") / fname;
                    if (!save_maze_json(out, map, entrance, goal_cell, entrance_heading, mi)) {
                        std::fprintf(stderr, "Falha ao salvar %s\n", out.string().c_str());
                        push_log("Erro ao salvar novo labirinto.", SDL_Color{230,160,160,255});
                    } else {
                        push_log(std::string("Novo labirinto salvo: ") + out.string(), SDL_Color{180,220,180,255});
                    }
                    start = entrance; goal = goal_cell; agent = start; heading = entrance_heading;
                    nav.setMapDimensions(W, H);
                    nav.setStartGoal(start, goal);
                    nav.map() = map;
                    nav.planRoute();
                    steps = 0; collisions = 0; paused = false; last_step = SDL_GetTicks();
                    phase = Phase::Ready;
                    btnStart.label = "Iniciar"; btnStart.enabled = true;
                    btnNew.enabled = true; // disponível sempre
                }
            }
        }

        // avanço baseado no Navigator e plano (fallback RightHand)
        Uint32 now = SDL_GetTicks();
        if (!paused && (now - last_step > 250) && (phase==Phase::RunningExplore || phase==Phase::RunningReplay)) {
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
                    push_log("FWD: recompensa +1", SDL_Color{150,220,150,255});
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
                    push_log("COLISÃO: punição -5 (replanejar)", SDL_Color{220,150,150,255});
                }
            } else {
                apply_move(agent, heading, dec.action);
                moved = true;
                if (dec.action==maze::Action::Left) push_log("LEFT: custo -0.1", SDL_Color{200,200,150,255});
                else if (dec.action==maze::Action::Right) push_log("RIGHT: custo -0.1", SDL_Color{200,200,150,255});
                else if (dec.action==maze::Action::Back) push_log("BACK: custo -0.2", SDL_Color{200,180,150,255});
            }
            if (moved) steps++;
            if (agent.x==goal.x && agent.y==goal.y) {
                float sim_time_s = (SDL_GetTicks() - start_ms) / 1000.0f;
                int cost = steps + collisions * 5;
                std::printf("Reached goal in %d steps, collisions=%d, time=%.2fs, cost=%d\n", steps, collisions, sim_time_s, cost);
                // Freeze timer on success
                frozen_ms = SDL_GetTicks() - start_ms;
                time_frozen = true;
                paused = true;
                phase = Phase::FinishedSuccess;
                btnStart.label = "Iniciar"; // for replay
                btnNew.enabled = true; // only on success per requirement
                push_log("Objetivo alcançado!", SDL_Color{160,220,160,255});
            }
            if (steps > max_steps_fail && phase==Phase::RunningExplore) {
                paused = true; phase = Phase::FinishedFail; btnStart.label = "Teste"; btnNew.enabled = true; push_log("Falha: sem solução (limite)", SDL_Color{220,160,160,255});
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        // Left drawing area (exclude sidebar)
        draw_grid(ren, OX, OY, CELL, W, H);
        draw_maze(ren, map, OX, OY, CELL);
        draw_agent(ren, agent, heading, OX, OY, CELL);
        float sim_time_s = time_frozen ? (frozen_ms / 1000.0f) : ((SDL_GetTicks() - start_ms) / 1000.0f);
        int cost = steps + collisions * 5;
        char title[160];
        std::snprintf(title, sizeof(title), "Maze Simulator - steps=%d col=%d time=%.1fs cost=%d %s", steps, collisions, sim_time_s, cost, paused?"(paused)":"");
        SDL_SetWindowTitle(win, title);
        // Sidebar
        draw_sidebar(ren, font, sidebar, log, (win_h-200)/18);
        // Buttons
        draw_button(ren, font, btnStart);
        draw_button(ren, font, btnNew);
        SDL_RenderPresent(ren);
    }
    ui_font_destroy(font);
#ifdef HAVE_SDL_TTF
    if (TTF_WasInit()) TTF_Quit();
#endif
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
