#include "unity.h"
#include "core/Planner.hpp"

using namespace maze;

void setUp() {}
void tearDown() {}

static MazeMap small_open_map() {
    MazeMap m(4,3); // 4x3 grid, no internal walls by default
    // Close outer borders
    for (int x=0;x<4;++x){ m.set_wall(x,0,'N',true); m.set_wall(x,2,'S',true);} 
    for (int y=0;y<3;++y){ m.set_wall(0,y,'W',true); m.set_wall(3,y,'E',true);} 
    return m;
}

void test_bfs_finds_path_in_open_map() {
    MazeMap m = small_open_map();
    auto path = Planner::bfs_path(m, {1,1}, {2,1});
    TEST_ASSERT_TRUE(path.has_value());
    TEST_ASSERT_GREATER_OR_EQUAL(2, (int)path->size());
    // Start and goal endpoints
    TEST_ASSERT_EQUAL_INT(1, path->front().x);
    TEST_ASSERT_EQUAL_INT(1, path->front().y);
    TEST_ASSERT_EQUAL_INT(2, path->back().x);
    TEST_ASSERT_EQUAL_INT(1, path->back().y);
}

void test_bfs_respects_walls() {
    MazeMap m = small_open_map();
    // Add a wall between (1,1) and (2,1)
    m.set_wall(1,1,'E',true);
    auto direct = Planner::bfs_path(m, {1,1}, {2,1});
    TEST_ASSERT_TRUE(direct.has_value());
    // Path length must be > 2 since the direct edge is blocked
    TEST_ASSERT_GREATER_OR_EQUAL(3, (int)direct->size());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_bfs_finds_path_in_open_map);
    RUN_TEST(test_bfs_respects_walls);
    return UNITY_END();
}
