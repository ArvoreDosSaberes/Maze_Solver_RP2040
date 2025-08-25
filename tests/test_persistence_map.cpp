/**
 * @file tests/test_persistence_map.cpp
 * @brief Testes de snapshot de mapa em `PersistentMemory` (save/load e mismatch).
 *
 * Valida a capacidade de salvar um snapshot do mapa e recarregá-lo com
 * fidelidade, além de checar falha prevista quando as dimensões divergem.
 *
 * Como executar:
 * - Via CTest: `ctest -R test_persistence_map`
 * - Ou executando o binário deste teste diretamente.
 */
#include "core/PersistentMemory.hpp"
#include "core/MazeMap.hpp"
#include "unity.h"

using namespace maze;

static void set_some_walls(MazeMap& m) {
    // Draw a small pattern to validate round-trip
    m.set_wall(0, 0, 'N', true);
    m.set_wall(0, 0, 'E', true);
    m.set_wall(1, 0, 'S', true);
    m.set_wall(1, 1, 'W', true);
    m.set_wall(2, 2, 'N', true);
}

static void expect_same_maps(const MazeMap& a, const MazeMap& b) {
    TEST_ASSERT_EQUAL_INT(a.width(), b.width());
    TEST_ASSERT_EQUAL_INT(a.height(), b.height());
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const Cell& ca = a.at(x, y);
            const Cell& cb = b.at(x, y);
            TEST_ASSERT_EQUAL_INT(ca.wall_n, cb.wall_n);
            TEST_ASSERT_EQUAL_INT(ca.wall_e, cb.wall_e);
            TEST_ASSERT_EQUAL_INT(ca.wall_s, cb.wall_s);
            TEST_ASSERT_EQUAL_INT(ca.wall_w, cb.wall_w);
        }
    }
}

void setUp() {}
void tearDown() {}

static void test_snapshot_roundtrip(void) {
    // Clean any previous files to avoid interference
    (void)PersistentMemory::eraseAll();

    MazeMap m1(4, 4);
    set_some_walls(m1);

    bool saved = PersistentMemory::saveMapSnapshot(m1);
    TEST_ASSERT_TRUE_MESSAGE(saved, "saveMapSnapshot should succeed");

    MazeMap m2(4, 4);
    bool loaded = PersistentMemory::loadMapSnapshot(&m2);
    TEST_ASSERT_TRUE_MESSAGE(loaded, "loadMapSnapshot should succeed");

    expect_same_maps(m1, m2);
}

static void test_snapshot_dimension_mismatch(void) {
    // Ensure there is a snapshot saved with 4x4
    MazeMap m1(4, 4);
    set_some_walls(m1);
    TEST_ASSERT_TRUE(PersistentMemory::saveMapSnapshot(m1));

    // Try to load into a 5x5 map; should fail
    MazeMap wrong(5, 5);
    bool ok = PersistentMemory::loadMapSnapshot(&wrong);
    TEST_ASSERT_FALSE_MESSAGE(ok, "loadMapSnapshot should fail for dim mismatch");
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_roundtrip);
    RUN_TEST(test_snapshot_dimension_mismatch);
    return UNITY_END();
}
