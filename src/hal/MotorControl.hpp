/**
 * @file MotorControl.hpp
 * @brief Interface do controle de dois motores (esquerdo/direito) com mapeamento
 *        de velocidades normalizadas e direção.
 *
 * Esta interface abstrai o controle de dois motores de tração diferencial. A
 * implementação concreta é selecionada no CMake, por exemplo, uma implementação
 * baseada em PWM e ponte H.
 *
 * Notas de uso:
 * - Velocidades são valores normalizados em [-1.0 .. 1.0]. O sinal indica
 *   direção: negativo = ré, positivo = frente.
 * - Implementações devem saturar e clamp nos limites e lidar com rampa de
 *   aceleração/desaceleração conforme necessário (para proteger hardware).
 * - A função `arcadeDrive()` converte componentes de avanço/rotação em
 *   velocidades de cada lado (mixing), também em [-1..1].
 * - A função `stop()` deve comandar saída neutra/zero e deixar os motores em
 *   estado seguro (sem PWM ou duty mínimo), respeitando a eletrônica usada.
 *
 * Troque a implementação fornecendo outro .cpp no build (variável CMake
 * `MOTOR_IMPL`).
 *
 * @since 0.1
 */
#pragma once
#include <cstdint>

namespace hal {

/**
 * @brief Controla dois motores (esquerdo e direito) de tração diferencial.
 *
 * Responsabilidades:
 * - Inicializar GPIOs/pinos e periféricos (ex.: PWM) conforme a implementação.
 * - Aplicar comandos de velocidade normalizada com saturação e direção.
 * - Oferecer modo arcade (avançar/rotacionar) para navegação simples.
 *
 * Thread-safety: não é intrinsecamente thread-safe. Se houver acesso concorrente
 * a partir de ISRs/threads, proteger externamente (mutex/desabilitar IRQs).
 */
class MotorControl {
public:
    /**
     * @brief Constrói o controlador e realiza configuração de pinos/periféricos.
     * @param l_pwm  GPIO/pino associado ao PWM do motor esquerdo.
     * @param l_dirA GPIO/pino auxiliar A do motor esquerdo (ex.: entrada ponte H).
     * @param l_dirB GPIO/pino auxiliar B do motor esquerdo (ex.: entrada ponte H).
     * @param r_pwm  GPIO/pino associado ao PWM do motor direito.
     * @param r_dirA GPIO/pino auxiliar A do motor direito.
     * @param r_dirB GPIO/pino auxiliar B do motor direito.
     *
     * Detalhes de mapeamento dependem da implementação .cpp selecionada (ex.:
     * quais combinações de dirA/dirB indicam frente/ré e como o PWM é aplicado).
     */
    MotorControl(uint8_t l_pwm, uint8_t l_dirA, uint8_t l_dirB,
                 uint8_t r_pwm, uint8_t r_dirA, uint8_t r_dirB);

    /**
     * @brief Define velocidade do motor esquerdo.
     * @param v Velocidade normalizada no intervalo [-1.0 .. 1.0].
     * @note Valores fora do intervalo serão saturados. Sinal define direção.
     */
    void setSpeedLeft(float v);

    /**
     * @brief Define velocidade do motor direito.
     * @param v Velocidade normalizada no intervalo [-1.0 .. 1.0].
     * @note Valores fora do intervalo serão saturados. Sinal define direção.
     */
    void setSpeedRight(float v);

    /**
     * @brief Para ambos os motores, colocando a saída em estado seguro/neutro.
     * @details Implementações podem zerar PWM e definir direção para coasting ou
     *          brake leve, conforme eletrônica. Evitar ruídos/glitches.
     */
    void stop();

    /**
     * @brief Controle arcade: combina avanço e rotação.
     * @param forward Componente de avanço em [-1.0 .. 1.0].
     * @param rotate  Componente de rotação em [-1.0 .. 1.0] (positivo gira à dir.).
     * @details Realiza mixing padrão: left = forward + rotate; right = forward - rotate;
     *          aplica normalização/saturação para manter [-1..1] e envia aos motores.
     */
    void arcadeDrive(float forward, float rotate);

private:
    // Internos dependentes da implementação concreta no .cpp
    void setup_impl(uint8_t l_pwm, uint8_t l_dirA, uint8_t l_dirB,
                    uint8_t r_pwm, uint8_t r_dirA, uint8_t r_dirB);
    void apply_left(float v);
    void apply_right(float v);

    uint8_t l_pwm_, l_dirA_, l_dirB_;
    uint8_t r_pwm_, r_dirA_, r_dirB_;
};

} // namespace hal
