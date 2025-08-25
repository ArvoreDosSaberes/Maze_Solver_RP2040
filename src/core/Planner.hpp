#pragma once
#include <vector>
#include <algorithm>
#include <queue>
#include <optional>
#include <cstdint>
#include "MazeMap.hpp"

/**
 * @file Planner.hpp
 * @brief Planejador de caminho em grade usando BFS sobre `MazeMap`.
 */

namespace maze {

/**
 * @brief Planejador simples baseado em BFS (largura) no grafo implícito do labirinto.
 */
class Planner {
public:
    /**
     * @brief Encontra um caminho do início ao objetivo usando BFS.
     *
     * @param map  referência ao mapa do labirinto
     * @param start célula inicial
     * @param goal  célula objetivo
     * @return sequência de pontos incluindo início e objetivo, ou std::nullopt se inalcançável
     */
    static std::optional<std::vector<Point>> bfs_path(const MazeMap& map, Point start, Point goal) {
        const int w = map.width();
        const int h = map.height();
        if (!map.in_bounds(start.x, start.y) || !map.in_bounds(goal.x, goal.y)) return std::nullopt;
        std::vector<int> prev(w*h, -1);
        std::vector<uint8_t> visited(w*h, 0);
        auto idx = [&](int x,int y){ return y*w + x; };
        std::queue<Point> q;
        q.push(start);
        visited[idx(start.x,start.y)] = 1;

        while(!q.empty()){
            Point p = q.front(); q.pop();
            if (p.x==goal.x && p.y==goal.y) break;
            const Cell& c = map.at(p.x,p.y);
            // N
            if (!c.wall_n && map.in_bounds(p.x, p.y-1)) {
                int j = idx(p.x, p.y-1); if(!visited[j]){ visited[j]=1; prev[j]=idx(p.x,p.y); q.push({p.x,p.y-1}); }
            }
            // E
            if (!c.wall_e && map.in_bounds(p.x+1, p.y)) {
                int j = idx(p.x+1, p.y); if(!visited[j]){ visited[j]=1; prev[j]=idx(p.x,p.y); q.push({p.x+1,p.y}); }
            }
            // S
            if (!c.wall_s && map.in_bounds(p.x, p.y+1)) {
                int j = idx(p.x, p.y+1); if(!visited[j]){ visited[j]=1; prev[j]=idx(p.x,p.y); q.push({p.x,p.y+1}); }
            }
            // W
            if (!c.wall_w && map.in_bounds(p.x-1, p.y)) {
                int j = idx(p.x-1, p.y); if(!visited[j]){ visited[j]=1; prev[j]=idx(p.x,p.y); q.push({p.x-1,p.y}); }
            }
        }
        if (!visited[idx(goal.x,goal.y)]) return std::nullopt;
        std::vector<Point> path;
        for (int cur = idx(goal.x,goal.y); cur!=-1; cur = prev[cur]) {
            int x = cur % w; int y = cur / w; path.push_back({x,y});
            if (cur == idx(start.x,start.y)) break;
        }
        std::reverse(path.begin(), path.end()); // reconstrói do goal ao start
        return path;
    }
};

} // namespace maze
