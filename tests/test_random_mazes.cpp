#include "unity.h"
#include "core/Planner.hpp"
#include <vector>
#include <random>
#include <stack>

using namespace maze;

static void add_all_walls(MazeMap& m) {
    const int w = m.width();
    const int h = m.height();
    // Close outer borders
    for (int x=0;x<w;++x){ m.set_wall(x,0,'N',true); m.set_wall(x,h-1,'S',true);} 
    for (int y=0;y<h;++y){ m.set_wall(0,y,'W',true); m.set_wall(w-1,y,'E',true);} 
    // Add internal walls between all adjacent cells to form a grid of closed cells
    for (int y=0;y<h;++y){
        for (int x=0;x<w;++x){
            if (y>0) m.set_wall(x,y,'N',true);
            if (x<w-1) m.set_wall(x,y,'E',true);
            if (y<h-1) m.set_wall(x,y,'S',true);
            if (x>0) m.set_wall(x,y,'W',true);
        }
    }
}

static void carve_maze_dfs(MazeMap& m, int sx, int sy, std::mt19937& rng) {
    const int w = m.width();
    const int h = m.height();
    std::vector<uint8_t> vis(w*h, 0);
    auto idx = [&](int x,int y){ return y*w + x; };
    std::stack<Point> st;
    st.push({sx,sy});
    vis[idx(sx,sy)] = 1;
    std::uniform_int_distribution<int> dist(0,3);
    while(!st.empty()){
        Point p = st.top();
        // Collect unvisited neighbors
        std::vector<std::pair<Point,char>> nbrs;
        if (p.y>0 && !vis[idx(p.x,p.y-1)]) nbrs.push_back({Point{p.x,p.y-1}, 'N'});
        if (p.x<w-1 && !vis[idx(p.x+1,p.y)]) nbrs.push_back({Point{p.x+1,p.y}, 'E'});
        if (p.y<h-1 && !vis[idx(p.x,p.y+1)]) nbrs.push_back({Point{p.x,p.y+1}, 'S'});
        if (p.x>0 && !vis[idx(p.x-1,p.y)]) nbrs.push_back({Point{p.x-1,p.y}, 'W'});
        if (nbrs.empty()) { st.pop(); continue; }
        std::shuffle(nbrs.begin(), nbrs.end(), rng);
        auto [q,dir] = nbrs.front();
        // Remove wall between p and q
        m.set_wall(p.x, p.y, dir, false);
        vis[idx(q.x,q.y)] = 1;
        st.push(q);
    }
}

static MazeMap gen_perfect_maze(int w, int h, uint32_t seed){
    MazeMap m(w,h);
    add_all_walls(m);
    std::mt19937 rng(seed);
    carve_maze_dfs(m, 0, 0, rng);
    return m;
}

void setUp() {}
void tearDown() {}

void test_bfs_on_random_mazes(){
    const int W=8, H=6;
    for (int i=0;i<4;++i){
        MazeMap m = gen_perfect_maze(W,H, 12345u + i);
        auto path = Planner::bfs_path(m, {0,0}, {W-1,H-1});
        TEST_ASSERT_TRUE_MESSAGE(path.has_value(), "Path should exist in perfect maze");
        TEST_ASSERT_GREATER_OR_EQUAL(2, (int)path->size());
    }
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_bfs_on_random_mazes);
    return UNITY_END();
}
