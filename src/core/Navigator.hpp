/**
 * @file Navigator.hpp
 * @brief Núcleo de decisão de navegação (plataforma-agnóstico).
 */
#pragma once
#include <cstdint>
#include <vector>
#include "MazeMap.hpp"
#include "Planner.hpp"
#include "Learning.hpp"

namespace maze {

/** @brief Ação possível do robô sobre a malha do labirinto. */
enum class Action : uint8_t { Right, Forward, Left, Back };

/** @brief Leituras discretizadas dos sensores de obstáculos. */
struct SensorRead {
    bool left_free{false};   ///< true se não há obstáculo à esquerda
    bool front_free{false};  ///< true se não há obstáculo à frente
    bool right_free{false};  ///< true se não há obstáculo à direita
};

/**
 * @brief Decisão calculada pelo navegador.
 *
 * "score" reflete quão boa é a ação [0..10] conforme heurística/planejamento atual.
 */
struct Decision {
    Action action{Action::Forward}; ///< Ação escolhida
    uint8_t score{6};               ///< Nota de 0..10 para a ação
};

/**
 * @brief Estratégias de navegação disponíveis.
 */
class Navigator {
public:
    /// Estratégia ativa
    enum class Strategy : uint8_t { RightHand /* Mão Direita */ };

    /**
     * @brief Define a estratégia de navegação.
     * @param s estratégia desejada
     */
    void setStrategy(Strategy s) { strategy_ = s; }

    /**
     * @brief Decide próxima ação a partir de leituras de sensores.
     * @param sr leituras discretizadas (livre/ocupado)
     * @return Decisão com ação e nota [0..10]
     */
    Decision decide(const SensorRead& sr);

    // ---------- Integração com mapa e planner (opcionais) ----------
    /** @brief Define as dimensões do mapa interno e reinicia estatísticas de visita. */
    void setMapDimensions(int w, int h) {
        map_ = MazeMap(w,h);
        seen_.assign(w * h, 0);
    }
    /** @brief Define célula inicial e objetivo e habilita o estado de objetivo. */
    void setStartGoal(Point s, Point g) { start_ = s; goal_ = g; has_goal_ = true; }

    /**
     * @brief Observa paredes a partir das leituras e orientação atual.
     * @param cell célula atual (x,y)
     * @param sr leituras discretizadas
     * @param heading orientação atual (0=N,1=E,2=S,3=W)
     */
    void observeCellWalls(Point cell, const SensorRead& sr, uint8_t heading);
    /** @brief Planeja rota do start ao goal. @return true se uma rota foi encontrada */
    bool planRoute();
    /** @brief Indica se há um plano válido armazenado. */
    bool hasPlan() const { return !plan_.empty(); }

    /**
     * @brief Acessa a sequência de pontos do plano atual (somente leitura).
     *
     * Útil para visualização em simuladores. Retorna uma referência const ao
     * vetor interno; será vazio quando não houver plano.
     */
    const std::vector<Point>& currentPlan() const { return plan_; }

    /**
     * @brief Decide considerando rota planejada (se existir); senão, fallback RightHand.
     * @param current célula atual
     * @param heading orientação atual (0=N,1=E,2=S,3=W)
     * @param sr leituras discretizadas
     * @return Decisão com ação e nota [0..10]
     */
    Decision decidePlanned(Point current, uint8_t heading, const SensorRead& sr);

    // ---------- Heurísticas de aprendizado ----------
    /** @brief Define as heurísticas internas. */
    void setHeuristics(const Heuristics& h) { heur_ = h; }
    /** @brief Retorna uma cópia das heurísticas internas. */
    Heuristics heuristics() const { return heur_; }
    /** @brief Aplica recompensa a uma ação tomada (atualiza heurísticas). */
    void applyReward(Action a, float reward);

    // ---------- Acesso ao mapa para persistência ----------
    /** @brief Acesso ao mapa interno para leitura/escrita. */
    MazeMap& map() { return map_; }
    /** @brief Acesso somente-leitura ao mapa interno. */
    const MazeMap& map() const { return map_; }

private:
    Strategy strategy_{Strategy::RightHand}; ///< Estratégia atual
    // Estado do mapa/rota
    MazeMap map_{1,1};                    ///< Mapa conhecido
    Point start_{0,0};                    ///< Célula inicial
    Point goal_{0,0};                     ///< Célula objetivo
    bool has_goal_{false};                ///< Indica se goal foi definido
    std::vector<Point> plan_{};           ///< Sequência de células (inclui start e goal)

    Heuristics heur_{};                   ///< Pesos para ações

    /** @brief Contador de visitas por célula (para explorar novidades primeiro). */
    std::vector<uint8_t> seen_{};
    /** @brief Índice linear em `seen_`. */
    inline int idx(int x, int y) const { return y * map_.width() + x; }

    /** @brief Calcula nota para uma ação dado o estado sensorial. */
    uint8_t score_for(Action a, const SensorRead& sr) const;
};

} // namespace maze
