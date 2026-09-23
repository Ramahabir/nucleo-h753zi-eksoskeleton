/**
  ******************************************************************************
  * @file    fsr.h
  * @brief   Driver for Force Sensitive Resistor (FSR) sensors.
  *          Provides ADC sampling, EMA low-pass filtering, voltage conversion,
  *          and hysteresis-based gait contact state detection (stance/swing).
  ******************************************************************************
  */

#ifndef __FSR_H__
#define __FSR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief FSR contact state (Stance vs Swing)
 */
typedef enum {
    FSR_STATE_AIRBORNE = 0,   /*!< Foot in swing phase / no ground contact detected */
    FSR_STATE_CONTACT  = 1    /*!< Foot in stance phase / ground contact confirmed */
} FSR_State_t;

/**
 * @brief FSR instance configuration and telemetry data
 */
typedef struct {
    ADC_HandleTypeDef *hadc;       /*!< Associated STM32 HAL ADC handle */

    // Calibration & threshold configuration
    uint16_t contact_threshold;   /*!< Raw ADC threshold to trigger CONTACT state (heel-strike) */
    uint16_t release_threshold;   /*!< Raw ADC threshold to return to AIRBORNE state (toe-off) */
    float    filter_alpha;        /*!< Exponential Moving Average factor (0.0 < alpha <= 1.0) */

    // Live measurement data
    uint16_t    raw_adc;          /*!< Latest raw 16-bit ADC sample (0 - 65535) */
    float       filtered_adc;     /*!< Low-pass filtered ADC value */
    float       voltage;          /*!< Calculated analog voltage in Volts (0.0V - 3.3V) */
    FSR_State_t state;            /*!< Current gait contact state */
    bool        state_changed;    /*!< True for 1 cycle when state transitions occur */
} FSR_HandleTypeDef;

/**
 * @brief  Initialize the FSR driver handle.
 * @param  hfsr: Pointer to FSR_HandleTypeDef
 * @param  hadc: Pointer to configured ADC_HandleTypeDef
 * @param  contact_thresh: Upper ADC threshold for contact detection (e.g. 15000)
 * @param  release_thresh: Lower ADC threshold for contact release (e.g. 10000)
 * @param  filter_alpha: EMA smoothing coefficient (typically 0.2 to 0.4)
 */
void FSR_Init(FSR_HandleTypeDef *hfsr, ADC_HandleTypeDef *hadc,
              uint16_t contact_thresh, uint16_t release_thresh, float filter_alpha);

/**
 * @brief  Execute a single ADC conversion and return the raw 16-bit result.
 * @param  hfsr: Pointer to FSR_HandleTypeDef
 * @retval 16-bit raw ADC reading
 */
uint16_t FSR_ReadRaw(FSR_HandleTypeDef *hfsr);

/**
 * @brief  Sample the sensor, apply EMA filtering, calculate voltage, and evaluate
 *         contact state with hysteresis.
 * @param  hfsr: Pointer to FSR_HandleTypeDef
 */
void FSR_Update(FSR_HandleTypeDef *hfsr);

/**
 * @brief  Check if ground contact is currently active.
 * @param  hfsr: Pointer to FSR_HandleTypeDef
 * @retval true if CONTACT (stance), false if AIRBORNE (swing)
 */
static inline bool FSR_IsContact(const FSR_HandleTypeDef *hfsr) {
    return (hfsr != NULL) && (hfsr->state == FSR_STATE_CONTACT);
}

#ifdef __cplusplus
}
#endif

#endif /* __FSR_H__ */
