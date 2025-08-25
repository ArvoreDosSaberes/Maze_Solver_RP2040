/**
 * @file tests/test_navigator.cpp
 * @brief Teste unitário do comportamento "Right-Hand" do Navigator (sem plano).
 *
 * Verifica a ordem de preferência: direita > frente > esquerda > ré quando as
 * respectivas opções estão livres/ocupadas de forma isolada.
 *
 * Como executar:
 * - Via CTest: `ctest -R test_navigator`
 * - Ou executando diretamente o binário deste teste.
 */
#include "unity.h"
#include "core/Navigator.hpp"
#include <cstdlib>

using namespace maze;

void setUp() {}
void tearDown() {}

static SensorRead make(bool l, bool f, bool r) {
    SensorRead sr; sr.left_free=l; sr.front_free=f; sr.right_free=r; return sr;
}

void test_right_hand_prefers_right_then_front_then_left_then_back() {
    Navigator nav; nav.setStrategy(Navigator::Strategy::RightHand);

    // right free
    auto d1 = nav.decide(make(false,false,true));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Right, (uint8_t)d1.action);

    // front free
    auto d2 = nav.decide(make(false,true,false));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Forward, (uint8_t)d2.action);

    // left free
    auto d3 = nav.decide(make(true,false,false));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Left, (uint8_t)d3.action);

    // none free -> back
    auto d4 = nav.decide(make(false,false,false));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)Action::Back, (uint8_t)d4.action);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_right_hand_prefers_right_then_front_then_left_then_back);
    return UNITY_END();
}
