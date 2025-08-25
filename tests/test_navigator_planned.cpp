/**
 * @file tests/test_navigator_planned.cpp
 * @brief Testes do `Navigator::decidePlanned()` seguindo uma rota planejada.
 *
 * Garante que quando a orientação atual coincide com o próximo passo do plano,
 * a ação seja `Forward`, e que giros relativos corretos sejam escolhidos quando
 * necessário para alinhar com o plano.
 *
 * Como executar:
 * - Via CTest: `ctest -R test_navigator_planned`
 * - Ou execute diretamente o binário gerado deste teste.
 */
#include "unity.h"
#include "core/Navigator.hpp"

using namespace maze;

void setUp() {}
void tearDown() {}

static SensorRead free_all() { SensorRead sr; sr.left_free=sr.front_free=sr.right_free=true; return sr; }

void test_decidePlanned_follows_forward_when_heading_matches() {
    Navigator nav; nav.setStrategy(Navigator::Strategy::RightHand);
    nav.setMapDimensions(3,1);
    nav.setStartGoal({0,0},{2,0});
    // No walls: plan should be (0,0)->(1,0)->(2,0)
    TEST_ASSERT_TRUE(nav.planRoute());

    // At (0,0), heading East (1). Next is (1,0) -> should be Forward
    auto d = nav.decidePlanned({0,0}, /*heading=*/1, free_all());
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Forward, (uint8_t)d.action);
}

void test_decidePlanned_turns_right_when_needed() {
    Navigator nav; nav.setStrategy(Navigator::Strategy::RightHand);
    nav.setMapDimensions(2,2);
    nav.setStartGoal({0,0},{1,0});
    TEST_ASSERT_TRUE(nav.planRoute());

    // At (0,1) heading North (0), but our plan only includes start (0,0). For a controlled case,
    // use (0,0)->(1,0) with heading North, desired absolute is East => relative Right.
    auto d = nav.decidePlanned({0,0}, /*heading=*/0, free_all());
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Right, (uint8_t)d.action);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_decidePlanned_follows_forward_when_heading_matches);
    RUN_TEST(test_decidePlanned_turns_right_when_needed);
    return UNITY_END();
}
