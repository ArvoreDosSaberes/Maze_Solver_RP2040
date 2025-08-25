#include "Navigator.hpp"
#include <algorithm>

namespace maze {

/**
 * @brief Calcula a pontuação de preferência para uma ação dada a leitura de sensores.
 *
 * Converte os pesos das heurísticas em uma escala inteira simples (0..10),
 * penalizando direções bloqueadas e favorecendo aberturas conforme `heur_`.
 *
 * @param a ação candidata (`Action`)
 * @param sr leitura dos sensores indicando aberturas à esquerda/frente/direita
 * @return pontuação inteira na faixa [0,10]
 */
uint8_t Navigator::score_for(Action a, const SensorRead& sr) const {
    // Nota base ponderada pelas heurísticas e disponibilidade
    float base = 0.0f;
    switch (a) {
        case Action::Right:   base = sr.right_free ? heur_.w_right : 0.1f; break;
        case Action::Forward: base = sr.front_free ? heur_.w_front : 0.1f; break;
        case Action::Left:    base = sr.left_free  ? heur_.w_left  : 0.1f; break;
        case Action::Back:    base = (!sr.left_free && !sr.front_free && !sr.right_free) ? heur_.w_back : 0.2f; break;
    }
    // Mapear pesos (~0.2..3.0) para faixa 0..10 simples
    float score = (base / 3.0f) * 10.0f;
    if (score < 0.f) score = 0.f;
    if (score > 10.f) score = 10.f;
    return static_cast<uint8_t>(score > 10.f ? 10 : (score < 0.f ? 0 : score));
}

/**
 * @brief Decide a próxima ação baseada na estratégia configurada.
 *
 * Atualmente implementa a estratégia RightHand: prioriza direita, depois frente,
 * depois esquerda; se todas bloqueadas, retorna trás.
 *
 * @param sr leitura dos sensores indicando aberturas
 * @return decisão contendo ação e pontuação estimada
 */
Decision Navigator::decide(const SensorRead& sr) {
    Decision d{};
    if (strategy_ == Strategy::RightHand) {
        if (sr.right_free) { d.action = Action::Right; }
        else if (sr.front_free) { d.action = Action::Forward; }
        else if (sr.left_free) { d.action = Action::Left; }
        else { d.action = Action::Back; }
    }
    d.score = score_for(d.action, sr);
    return d;
}

// heading: 0=N,1=E,2=S,3=W. SensorRead free flags indicate openings.
/**
 * @brief Observa as paredes da célula atual e atualiza o `MazeMap`.
 *
 * Mapeia esquerda/frente/direita relativas para N/E/S/W absolutas a partir do
 * `heading` fornecido e chama `MazeMap::set_wall()` para refletir as obstruções.
 *
 * @param cell coordenadas da célula atual
 * @param sr leitura de sensores (true indica livre)
 * @param heading orientação absoluta: 0=N,1=E,2=S,3=W
 */
void Navigator::observeCellWalls(Point cell, const SensorRead& sr, uint8_t heading) {
    auto set_dir = [&](char dir, bool free_flag){ map_.set_wall(cell.x, cell.y, dir, !free_flag); };
    // Map left/front/right relative to heading into absolute N/E/S/W
    // Order rel: Left, Front, Right
    const char abs_dirs[4] = {'N','E','S','W'};
    auto rel_to_abs = [&](int rel){ // 0=Left,1=Front,2=Right
        int base = (int)heading;
        int d = 0;
        if (rel==0) d = (base + 3) & 3; // left
        else if (rel==1) d = base;      // front
        else d = (base + 1) & 3;        // right
        return abs_dirs[d];
    };
    set_dir(rel_to_abs(0), sr.left_free);
    set_dir(rel_to_abs(1), sr.front_free);
    set_dir(rel_to_abs(2), sr.right_free);
}

/**
 * @brief Planeja uma rota do início ao objetivo usando `Planner::bfs_path`.
 *
 * Requer que um objetivo tenha sido definido. Ao sucesso, popula `plan_` com
 * a sequência de pontos do caminho.
 *
 * @return true se um plano não vazio foi gerado; false caso contrário
 */
bool Navigator::planRoute() {
    if (!has_goal_) return false;
    auto p = Planner::bfs_path(map_, start_, goal_);
    if (!p) { plan_.clear(); return false; }
    plan_ = *p;
    return !plan_.empty();
}

/**
 * @brief Decide ação seguindo o plano pré-computado, com fallback heurístico.
 *
 * Se `current` está no plano e existe um próximo ponto, converte a direção
 * absoluta desejada em ação relativa segundo o `heading` atual.
 * Caso o plano seja inválido ou não encontrado, retorna `decide(sr)` heurístico.
 *
 * @param current posição atual no mapa
 * @param heading orientação absoluta: 0=N,1=E,2=S,3=W
 * @param sr leitura dos sensores
 * @return decisão planejada (pontuação alta), ou heurística caso não aplicável
 */
Decision Navigator::decidePlanned(Point current, uint8_t heading, const SensorRead& sr) {
    if (!plan_.empty()) {
        // Find current index in plan
        auto it = std::find_if(plan_.begin(), plan_.end(), [&](const Point& pt){ return pt.x==current.x && pt.y==current.y; });
        if (it != plan_.end()) {
            if (std::next(it) != plan_.end()) {
                Point next = *std::next(it);
                // Determine absolute desired move dir
                char want;
                if (next.x == current.x && next.y == current.y-1) want = 'N';
                else if (next.x == current.x+1 && next.y == current.y) want = 'E';
                else if (next.x == current.x && next.y == current.y+1) want = 'S';
                else if (next.x == current.x-1 && next.y == current.y) want = 'W';
                else want = 'X';

                // Map absolute desired dir to relative action given heading
                auto to_rel = [&](char abs){
                    const char abs_dirs[4] = {'N','E','S','W'};
                    int base = (int)heading; // front abs index
                    int idx = -1;
                    for (int i=0;i<4;++i) if (abs_dirs[i]==abs) { idx=i; break; }
                    if (idx<0) return Action::Back; // unknown
                    // Compute relative: 0=Front,1=Right,2=Back,3=Left
                    int rel = (idx - base + 4) & 3;
                    if (rel==0) return Action::Forward;
                    if (rel==1) return Action::Right;
                    if (rel==3) return Action::Left;
                    return Action::Back;
                };
                if (want!='X') {
                    Action a = to_rel(want);
                    Decision d; d.action = a; d.score = 9; return d;
                }
            }
        }
    }
    // Fallback: original RightHand logic
    return decide(sr);
}

/**
 * @brief Aplica uma recompensa à heurística para a ação tomada.
 *
 * Converte `Action` para o índice esperado por `update_heuristic()` e ajusta
 * os pesos internos `heur_` com o valor de `reward`.
 *
 * @param a ação executada
 * @param reward recompensa aplicada (positiva ou negativa)
 */
void Navigator::applyReward(Action a, float reward) {
    // Converter Action para índice esperado por update_heuristic: 0=Right,1=Front,2=Left,3=Back
    uint8_t idx = 0;
    switch (a) {
        case Action::Right:   idx = 0; break;
        case Action::Forward: idx = 1; break;
        case Action::Left:    idx = 2; break;
        case Action::Back:    idx = 3; break;
    }
    update_heuristic(heur_, idx, reward);
}

} // namespace maze
