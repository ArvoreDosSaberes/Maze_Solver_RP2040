/**
 * @file IRSensorArray.cpp
 * @brief Implementação da leitura e filtragem dos sensores IR via ADC.
 */
#include "IRSensorArray.hpp"

#include "pico/stdlib.h"
#include "hardware/adc.h"

namespace hal {

/**
 * @brief Constrói o array de sensores IR, inicializando ADC e GPIOs associados.
 *
 * Canais válidos do ADC RP2040 são 0..4; quando 0..3, mapeiam para GPIO26..29.
 * GPIOs correspondentes são configurados para função ADC automaticamente.
 *
 * @param adc_left  canal ADC para sensor esquerdo (0..4)
 * @param adc_front canal ADC para sensor frontal (0..4)
 * @param adc_right canal ADC para sensor direito (0..4)
 */
IRSensorArray::IRSensorArray(uint8_t adc_left, uint8_t adc_front, uint8_t adc_right)
: ch_left_(adc_left), ch_front_(adc_front), ch_right_(adc_right) {
    adc_init();
    // Configure corresponding GPIOs to ADC function when channels map to GPIO26..29
    auto init_adc_gpio_if_valid = [](uint8_t ch){
        if (ch <= 3) { // ADC0..ADC3 map to GPIO26..GPIO29
            adc_gpio_init((uint) (26 + ch));
        }
    };
    init_adc_gpio_if_valid(ch_left_);
    init_adc_gpio_if_valid(ch_front_);
    init_adc_gpio_if_valid(ch_right_);
}

/**
 * @brief Lê um canal do ADC e normaliza o valor para [0,1].
 * @param ch canal ADC a ser lido
 * @return valor normalizado (float)
 */
static inline float read_adc_norm(uint8_t ch) {
    // Em RP2040, canais 0..4 correspondem a GPIO26..29 e interno
    adc_select_input(ch);
    uint16_t raw = adc_read(); // 12 bits efetivos
    return (float)raw / 4095.0f;
}

/**
 * @brief Lê todos os sensores e aplica filtragem exponencial (EMA).
 *
 * Na primeira chamada, inicializa o estado filtrado (`filt_`) com o valor bruto.
 * Nas subsequentes, aplica `alpha_` como fator de suavização.
 *
 * @return valores filtrados dos sensores esquerdo, frontal e direito
 */
IRValues IRSensorArray::readAll() const {
    IRValues raw{};
    raw.left  = read_adc_norm(ch_left_);
    raw.front = read_adc_norm(ch_front_);
    raw.right = read_adc_norm(ch_right_);

    if (!inited_) {
        filt_ = raw;
        inited_ = true;
        return filt_;
    }
    auto ema = [a = alpha_](float prev, float x){ return prev + a * (x - prev); };
    filt_.left  = ema(filt_.left,  raw.left);
    filt_.front = ema(filt_.front, raw.front);
    filt_.right = ema(filt_.right, raw.right);
    return filt_;
}

} // namespace hal
