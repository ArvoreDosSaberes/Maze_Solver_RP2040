#pragma once
#include <cstdint>

/**
 * @file Learning.hpp
 * @brief Estruturas e utilitários de heurística/aprendizado simples.
 */

namespace maze {

/**
 * @brief Pesos de preferência para cada ação possível.
 *
 * Os pesos influenciam a escolha da ação no navegador. Valores maiores indicam
 * maior tendência a escolher aquela direção quando liberada.
 */
struct Heuristics {
    float w_right{1.0f}; ///< Peso para virar à direita
    float w_front{1.0f}; ///< Peso para seguir em frente
    float w_left{1.0f};  ///< Peso para virar à esquerda
    float w_back{1.0f};  ///< Peso para dar ré (voltar)
};

/**
 * @brief Regra de atualização online simples baseada em recompensa.
 *
 * Aumenta ou diminui levemente o peso associado à ação executada, saturando
 * na faixa [0.2, 3.0].
 *
 * @param h       referência para a estrutura de heurísticas a ser atualizada
 * @param action  ação executada (0=right, 1=front, 2=left, 3=back)
 * @param reward  recompensa escalar (pode ser negativa)
 */
inline void update_heuristic(Heuristics& h, uint8_t action, float reward) {
    const float lr = 0.05f; // learning rate
    switch (action) {
        case 0: h.w_right += lr * reward; if (h.w_right < 0.2f) h.w_right = 0.2f; if (h.w_right > 3.0f) h.w_right = 3.0f; break;
        case 1: h.w_front += lr * reward; if (h.w_front < 0.2f) h.w_front = 0.2f; if (h.w_front > 3.0f) h.w_front = 3.0f; break;
        case 2: h.w_left  += lr * reward; if (h.w_left  < 0.2f) h.w_left  = 0.2f; if (h.w_left  > 3.0f) h.w_left  = 3.0f; break;
        case 3: h.w_back  += lr * reward; if (h.w_back  < 0.2f) h.w_back  = 0.2f; if (h.w_back  > 3.0f) h.w_back  = 3.0f; break;
    }
}

} // namespace maze
