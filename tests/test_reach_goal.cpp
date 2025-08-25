#include "unity.h"
#include "core/MazeMap.hpp"
#include "core/Navigator.hpp"
#include <vector>
#include <algorithm>
#include <random>

using namespace maze;

void setUp() {}
void tearDown() {}

static SensorRead make_sensor_read(const MazeMap& m, Point cell, uint8_t heading) {
    SensorRead sr{};
    const Cell& c = m.at(cell.x, cell.y);
    auto abs_left  = (heading + 3) & 3;
    auto abs_front = heading;
    auto abs_right = (heading + 1) & 3;
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

static void apply_move(Point& cell, uint8_t& heading, Action a) {
    if (a == Action::Left)  heading = (heading + 3) & 3;
    else if (a == Action::Right) heading = (heading + 1) & 3;
    else if (a == Action::Back) heading = (heading + 2) & 3;
    else {
        if (heading==0) cell.y -= 1;
        else if (heading==1) cell.x += 1;
        else if (heading==2) cell.y += 1;
        else cell.x -= 1;
    }
}

static void add_all_walls(MazeMap& m) {
    const int w = m.width();
    const int h = m.height();
    for (int x=0;x<w;++x){ m.set_wall(x,0,'N',true); m.set_wall(x,h-1,'S',true);} 
    for (int y=0;y<h;++y){ m.set_wall(0,y,'W',true); m.set_wall(w-1,y,'E',true);} 
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
    std::vector<Point> stack;
    stack.push_back({sx,sy});
    vis[idx(sx,sy)] = 1;
    while(!stack.empty()){
        Point p = stack.back();
        std::vector<std::pair<Point,char>> nbrs;
        if (p.y>0 && !vis[idx(p.x,p.y-1)]) nbrs.push_back({Point{p.x,p.y-1}, 'N'});
        if (p.x<w-1 && !vis[idx(p.x+1,p.y)]) nbrs.push_back({Point{p.x+1,p.y}, 'E'});
        if (p.y<h-1 && !vis[idx(p.x,p.y+1)]) nbrs.push_back({Point{p.x,p.y+1}, 'S'});
        if (p.x>0 && !vis[idx(p.x-1,p.y)]) nbrs.push_back({Point{p.x-1,p.y}, 'W'});
        if (nbrs.empty()) { stack.pop_back(); continue; }
        std::shuffle(nbrs.begin(), nbrs.end(), rng);
        auto [q,dir] = nbrs.front();
        m.set_wall(p.x, p.y, dir, false);
        vis[idx(q.x,q.y)] = 1;
        stack.push_back(q);
    }
}

static MazeMap gen_perfect_maze(int w, int h, uint32_t seed){
    MazeMap m(w,h);
    add_all_walls(m);
    std::mt19937 rng(seed);
    carve_maze_dfs(m, 0, 0, rng);
    return m;
}

static bool reach_goal_episode(const MazeMap& map, Navigator& nav, Point start, Point goal) {
    Point agent = start;
    uint8_t heading = 1; // East
    int guard = map.width()*map.height()*10;
    // initial plan
    nav.planRoute();
    while (guard-- > 0) {
        SensorRead sr = make_sensor_read(map, agent, heading);
        nav.observeCellWalls(agent, sr, heading);
        Decision d = nav.decidePlanned(agent, heading, sr);
        if (d.action == Action::Forward) {
            const char abs_dirs[4] = {'N','E','S','W'};
            char absdir = abs_dirs[heading];
            if (can_move(map, agent, absdir)) {
                apply_move(agent, heading, d.action);
            } else {
                // blocked: re-plan with current knowledge and try planned decision again
                nav.planRoute();
                d = nav.decidePlanned(agent, heading, sr);
                if (d.action == Action::Forward) {
                    // still forward but blocked per ground truth: choose a turn based on openings
                    // Prefer Right, then Left, else Back
                    if (sr.right_free) {
                        apply_move(agent, heading, Action::Right);
                    } else if (sr.left_free) {
                        apply_move(agent, heading, Action::Left);
                    } else {
                        apply_move(agent, heading, Action::Back);
                    }
                } else {
                    apply_move(agent, heading, d.action);
                }
            }
        } else {
            apply_move(agent, heading, d.action);
        }
        if (agent.x==goal.x && agent.y==goal.y) return true;
    }
    return false;
}

void test_agent_reaches_goal_in_random_mazes() {
    const int W=8, H=8;
    for (uint32_t i=0;i<4;++i) {
        MazeMap m = gen_perfect_maze(W,H, 9000u + i);
        Navigator nav; nav.setStrategy(Navigator::Strategy::RightHand);
        nav.setMapDimensions(W,H);
        Point start{0,0}, goal{W-1,H-1};
        nav.setStartGoal(start, goal);
        nav.planRoute();
        TEST_ASSERT_TRUE_MESSAGE(reach_goal_episode(m, nav, start, goal), "Agent failed to reach goal");
    }
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_agent_reaches_goal_in_random_mazes);
    return UNITY_END();
}
