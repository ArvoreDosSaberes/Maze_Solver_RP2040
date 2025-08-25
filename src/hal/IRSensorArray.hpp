/**
 * @file IRSensorArray.hpp
 * @brief Leitura de 3 sensores IR reflexivos analógicos via ADC.
 */
#pragma once
#include <cstdint>

namespace hal {

/** @brief Valores analógicos normalizados [0..1] dos três sensores. */
struct IRValues {
    float left{1.0f};   ///< Intensidade à esquerda (0..1)
    float front{1.0f};  ///< Intensidade à frente (0..1)
    float right{1.0f};  ///< Intensidade à direita (0..1)
};

/**
 * @brief Array de sensores IR conectados aos canais ADC do RP2040.
 *
 * Converte leituras para faixa [0..1], onde 0 ~ preto/baixa reflexão e 1 ~ alta reflexão.
 */
class IRSensorArray {
public:
    /**
     * @brief Constrói o array especificando entradas ADC.
     * @param adc_left  índice do canal ADC do sensor esquerdo (0..3, 26..28 se via GPIO mapping)
     * @param adc_front índice do canal ADC do sensor frontal
     * @param adc_right índice do canal ADC do sensor direito
     */
    IRSensorArray(uint8_t adc_left, uint8_t adc_front, uint8_t adc_right);

    /** @brief Lê os três sensores e retorna valores normalizados [0..1]. */
    IRValues readAll() const;

    /**
     * @brief Define o fator de suavização exponencial (EMA).
     *        alpha in (0,1]; valores menores suavizam mais.
     *        Ex.: para dt=150ms e tau~0.5s: alpha≈dt/(tau+dt)≈0.23.
     */
    void setSmoothing(float alpha) { alpha_ = (alpha <= 0.f) ? 1.f : (alpha > 1.f ? 1.f : alpha); }

private:
    uint8_t ch_left_, ch_front_, ch_right_; ///< Índices/canais ADC dos sensores
    float alpha_{0.25f};                    ///< Fator de suavização exponencial (EMA)
    mutable bool inited_{false};            ///< Flag de inicialização preguiçosa do hardware
    mutable IRValues filt_{};               ///< Valores filtrados mantidos entre leituras
};

} // namespace hal
