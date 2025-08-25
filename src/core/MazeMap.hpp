#pragma once
#include <vector>
#include <cstdint>

/**
 * @file MazeMap.hpp
 * @brief Estruturas e classe para representação de um labirinto em grade.
 */

namespace maze {

/**
 * @brief Célula do labirinto com presença/ausência de paredes nos quatro lados.
 *
 * Convenção de direções: N (norte), E (leste), S (sul), W (oeste).
 */
struct Cell {
    bool wall_n{false}; ///< Parede ao norte
    bool wall_e{false}; ///< Parede ao leste
    bool wall_s{false}; ///< Parede ao sul
    bool wall_w{false}; ///< Parede ao oeste
};

/**
 * @brief Ponto (coordenadas inteiras) na grade do labirinto.
 */
struct Point {
    int x{0}; ///< Coordenada x (coluna)
    int y{0}; ///< Coordenada y (linha)
};

/**
 * @brief Mapa de labirinto em grade (largura x altura) com acesso a paredes.
 */
class MazeMap {
public:
    /**
     * @brief Constrói um mapa vazio com dimensões fornecidas.
     * @param w largura (número de colunas)
     * @param h altura (número de linhas)
     */
    MazeMap(int w, int h) : w_(w), h_(h), grid_(w * h) {}

    /** @brief Retorna a largura do mapa. */
    int width() const { return w_; }
    /** @brief Retorna a altura do mapa. */
    int height() const { return h_; }
    /** @brief Verifica se (x,y) está dentro dos limites. */
    bool in_bounds(int x, int y) const { return x>=0 && y>=0 && x<w_ && y<h_; }

    /** @brief Acesso mutável à célula (x,y). */
    Cell& at(int x, int y) { return grid_[y * w_ + x]; }
    /** @brief Acesso somente-leitura à célula (x,y). */
    const Cell& at(int x, int y) const { return grid_[y * w_ + x]; }

    /**
     * @brief Define parede bidirecional entre (x,y) e seu vizinho na direção dada.
     * @param x coluna da célula base
     * @param y linha da célula base
     * @param dir 'N','E','S','W' indicando a direção do vizinho
     * @param present true para colocar parede, false para remover
     */
    void set_wall(int x, int y, char dir, bool present) {
        if (!in_bounds(x,y)) return;
        Cell& c = at(x,y);
        if (dir=='N') { c.wall_n = present; if (in_bounds(x,y-1)) at(x,y-1).wall_s = present; }
        else if (dir=='E') { c.wall_e = present; if (in_bounds(x+1,y)) at(x+1,y).wall_w = present; }
        else if (dir=='S') { c.wall_s = present; if (in_bounds(x,y+1)) at(x,y+1).wall_n = present; }
        else if (dir=='W') { c.wall_w = present; if (in_bounds(x-1,y)) at(x-1,y).wall_e = present; }
    }

private:
    int w_;                 ///< Largura em células
    int h_;                 ///< Altura em células
    std::vector<Cell> grid_;///< Armazenamento linear de células (linha-major)
};

} // namespace maze
