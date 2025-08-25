/**
 * @file MotorControl_HBridge.cpp
 * @brief Implementação de `hal::MotorControl` usando ponte H (2 pinos por motor).
 *
 * Mapeamento de pinos (compatível com a assinatura do construtor em `MotorControl.hpp`):
 * - Esquerda:  `l_pwm` -> IN1 (PWM), `l_dirA` -> IN2 (GPIO LOW/HIGH), `l_dirB` (não usado)
 * - Direita:   `r_pwm` -> IN1 (PWM), `r_dirA` -> IN2 (GPIO LOW/HIGH), `r_dirB` (não usado)
 *
 * Estratégia de controle:
 * - Avanço:    IN1 = PWM(|v|), IN2 = LOW
 * - Ré:        IN1 = LOW,      IN2 = HIGH (sinal simples de ré)
 * - Parar:     IN1 = LOW,      IN2 = LOW (coast)
 *
 * Notas de hardware:
 * - Frequência PWM: definida por `pwm_set_wrap(..., 65535)` e `pwm_set_clkdiv(4.0f)`. Ajuste conforme o driver/Ponte H
 *   para evitar zumbido e aquecimento. Para motores DC, 10–20 kHz é comum.
 * - Esta implementação usa IN2 como GPIO (LOW/HIGH). Se desejar frenagem ativa, IN2 pode ser canal PWM com duty > 0.
 * - Não há dead-time explícito; a ponte/driver deve prevenir cross-conduction. Se necessário, introduza tempos de
 *   comutação (desabilitar antes de inverter direção) conforme o hardware.
 * - Pinos `l_dirB`/`r_dirB` são ignorados aqui, mas preservados no construtor para compatibilidade.
 * - Segurança: `stop()` zera PWM e coloca IN2=LOW para coasting; ajuste se precisar de brake.
 *
 * @since 0.1
 */
#include "hal/MotorControl.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

namespace hal {

/**
 * @brief Limita um valor à faixa [0,1].
 */
static inline float clamp01(float x){ return x<0.f?0.f:(x>1.f?1.f:x);} 

/**
 * @brief Constrói e inicializa GPIO/PWM para a ponte H.
 * @param l_pwm  IN1 PWM do motor esquerdo.
 * @param l_dirA IN2 (GPIO) do motor esquerdo.
 * @param l_dirB Não usado nesta implementação.
 * @param r_pwm  IN1 PWM do motor direito.
 * @param r_dirA IN2 (GPIO) do motor direito.
 * @param r_dirB Não usado nesta implementação.
 */
MotorControl::MotorControl(uint8_t l_pwm, uint8_t l_dirA, uint8_t l_dirB,
                           uint8_t r_pwm, uint8_t r_dirA, uint8_t r_dirB)
: l_pwm_(l_pwm), l_dirA_(l_dirA), l_dirB_(l_dirB), r_pwm_(r_pwm), r_dirA_(r_dirA), r_dirB_(r_dirB) {
    (void)l_dirB_; (void)r_dirB_; // não usados nesta implementação
    setup_impl(l_pwm, l_dirA, 0, r_pwm, r_dirA, 0);
    stop();
}

/**
 * @brief Configura um pino GPIO como saída PWM com resolução de 16 bits.
 * @param gpio Número do pino GPIO.
 * @details Ajusta wrap para 65535 e divisor de clock 4.0. Ativa o slice.
 */
static void setup_pwm_pin(uint8_t gpio){
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 65535);
    pwm_set_clkdiv(slice, 4.0f);
    pwm_set_enabled(slice, true);
}

/**
 * @brief Implementação de setup dependente do hardware de ponte H.
 * @param l_pwm Pino PWM IN1 esquerdo.
 * @param l_in2 Pino IN2 esquerdo (GPIO saída, LOW para frente, HIGH para ré nesta impl.).
 * @param unused Reservado (não usado).
 * @param r_pwm Pino PWM IN1 direito.
 * @param r_in2 Pino IN2 direito (GPIO saída).
 * @param unused2 Reservado (não usado).
 */
void MotorControl::setup_impl(uint8_t l_pwm, uint8_t l_in2, uint8_t /*unused*/,
                              uint8_t r_pwm, uint8_t r_in2, uint8_t /*unused2*/){
    // Configura IN1 como PWM
    setup_pwm_pin(l_pwm);
    setup_pwm_pin(r_pwm);
    // IN2 como GPIO simples (coast/brake leve via LOW/HIGH). Pode ser estendido para PWM ativo.
    gpio_init(l_in2); gpio_set_dir(l_in2, GPIO_OUT); gpio_put(l_in2, 0);
    gpio_init(r_in2); gpio_set_dir(r_in2, GPIO_OUT); gpio_put(r_in2, 0);
}

/**
 * @brief Define o duty-cycle PWM normalizado [0..1] no pino dado.
 */
static inline void set_pwm(uint8_t gpio, float duty01){
    pwm_set_gpio_level(gpio, (uint16_t)(clamp01(duty01) * 65535.0f));
}

/**
 * @brief Aplica comando ao motor esquerdo.
 * @param v Velocidade normalizada [-1..1]. Sinal indica direção.
 * @details `v>=0`: IN1=PWM(|v|), IN2=LOW. `v<0`: IN1=0, IN2=HIGH (ré simples).
 */
void MotorControl::apply_left(float v){
    float mag = v < 0.f ? -v : v;
    if (v >= 0.f){
        // Avanço: IN1 = PWM, IN2 = 0
        set_pwm(l_pwm_, mag);
        gpio_put(l_dirA_, 0);
    } else {
        // Ré: IN1 = 0, IN2 = PWM(mag) -> se IN2 não for PWM, usar HIGH/LOW simples
        set_pwm(l_pwm_, 0.f);
        gpio_put(l_dirA_, 1); // sinal simples para ré; opcionalmente configurar PWM em outro pino
    }
}

/**
 * @brief Aplica comando ao motor direito.
 * @param v Velocidade normalizada [-1..1]. Sinal indica direção.
 */
void MotorControl::apply_right(float v){
    float mag = v < 0.f ? -v : v;
    if (v >= 0.f){
        set_pwm(r_pwm_, mag);
        gpio_put(r_dirA_, 0);
    } else {
        set_pwm(r_pwm_, 0.f);
        gpio_put(r_dirA_, 1);
    }
}

/** @copydoc MotorControl::setSpeedLeft */
void MotorControl::setSpeedLeft(float v){ apply_left(v); }
/** @copydoc MotorControl::setSpeedRight */
void MotorControl::setSpeedRight(float v){ apply_right(v); }

/**
 * @brief Para ambos os motores (coast), garantindo saída segura.
 */
void MotorControl::stop(){
    set_pwm(l_pwm_, 0.f);
    set_pwm(r_pwm_, 0.f);
    gpio_put(l_dirA_, 0);
    gpio_put(r_dirA_, 0);
}

/**
 * @brief Mixing arcade: left=forward+rotate, right=forward-rotate, com clamp.
 */
void MotorControl::arcadeDrive(float forward, float rotate){
    float left = forward + rotate;
    float right = forward - rotate;
    if (left > 1.f) left = 1.f; if (left < -1.f) left = -1.f;
    if (right > 1.f) right = 1.f; if (right < -1.f) right = -1.f;
    apply_left(left);
    apply_right(right);
}

} // namespace hal
