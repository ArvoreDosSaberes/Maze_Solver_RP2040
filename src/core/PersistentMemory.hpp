/**
 * @file PersistentMemory.hpp
 * @brief API de persistência de labirintos e estatísticas.
 */
#pragma once
#include <cstdint>
#include "Learning.hpp"
#include "MazeMap.hpp"

namespace maze {

/**
 * @brief Estatísticas/estado da memória persistente.
 */
struct PersistenceStatus {
    uint32_t saved_count{0};    ///< Quantidade de registros persistidos (heurísticas/mapa)
    uint32_t active_profile{0}; ///< Perfil/slot ativo (futuro uso para múltiplos perfis)
};

/**
 * @brief Fachada estática para acesso à memória persistente.
 *
 * Implementação real poderá usar flash (RP2040) ou arquivo (simulador).
 */
class PersistentMemory {
public:
    /**
     * @brief Apaga toda a base persistida (heurísticas e mapa), se existir.
     * @return true em caso de sucesso (ou arquivos inexistentes no host)
     */
    static bool eraseAll();

    /**
     * @brief Retorna status atual (contagens básicas) da persistência.
     * @return Estrutura com contagem de itens salvos e perfil ativo.
     */
    static PersistenceStatus status();

    /**
     * @brief Salva heurísticas aprendidas em armazenamento persistente.
     * @param h heurísticas a serem gravadas
     * @return true se persistiu (ou ao menos manteve em memória quando no host sem HOME)
     */
    static bool saveHeuristics(const Heuristics& h);

    /**
     * @brief Carrega heurísticas previamente salvas.
     * @param out ponteiro de saída para receber as heurísticas
     * @return true se carregou de persistência ou de fallback em memória; false se inexistente
     */
    static bool loadHeuristics(Heuristics* out);

    /**
     * @brief Salva um snapshot do mapa (paredes) compacto.
     *
     * Na plataforma RP2040, o snapshot coexiste no mesmo setor de flash onde
     * ficam as heurísticas, ocupando a segunda página.
     *
     * @param map referência ao mapa a ser serializado
     * @return true em caso de sucesso
     */
    static bool saveMapSnapshot(const MazeMap& map);

    /**
     * @brief Carrega snapshot do mapa para `out`.
     *
     * Requer que `out` tenha as mesmas dimensões salvas.
     * @param out ponteiro para mapa já alocado com mesmas dimensões
     * @return false se inexistente, dimensões divergentes ou erro de leitura
     */
    static bool loadMapSnapshot(MazeMap* out);
};

} // namespace maze
