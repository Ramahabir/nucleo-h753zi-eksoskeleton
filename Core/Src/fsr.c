/**
  ******************************************************************************
  * @file    fsr.c
  * @brief   Implementation of Force Sensitive Resistor (FSR) driver.
  ******************************************************************************
  */

#include "fsr.h"
#include <stddef.h>

#define FSR_VREF_VOLTAGE    3.3f
#define FSR_ADC_MAX_16B     65535.0f

void FSR_Init(FSR_HandleTypeDef *hfsr, ADC_HandleTypeDef *hadc,
              uint16_t contact_thresh, uint16_t release_thresh, float filter_alpha)
{
    if (hfsr == NULL) {
        return;
    }

    hfsr->hadc              = hadc;
    hfsr->contact_threshold = contact_thresh;
    hfsr->release_threshold = release_thresh;
    hfsr->filter_alpha      = (filter_alpha > 0.0f && filter_alpha <= 1.0f) ? filter_alpha : 0.3f;

    hfsr->raw_adc           = 0;
    hfsr->filtered_adc      = 0.0f;
    hfsr->voltage           = 0.0f;
    hfsr->state             = FSR_STATE_AIRBORNE;
    hfsr->state_changed     = false;
}

uint16_t FSR_ReadRaw(FSR_HandleTypeDef *hfsr)
{
    if (hfsr == NULL || hfsr->hadc == NULL) {
        return 0;
    }

    uint16_t raw_val = 0;

    if (HAL_ADC_Start(hfsr->hadc) == HAL_OK) {
        if (HAL_ADC_PollForConversion(hfsr->hadc, 10) == HAL_OK) {
            raw_val = (uint16_t)HAL_ADC_GetValue(hfsr->hadc);
        }
        HAL_ADC_Stop(hfsr->hadc);
    }

    return raw_val;
}

void FSR_Update(FSR_HandleTypeDef *hfsr)
{
    if (hfsr == NULL) {
        return;
    }

    // 1. Read raw ADC sample
    hfsr->raw_adc = FSR_ReadRaw(hfsr);

    // 2. Exponential Moving Average (EMA) low-pass filter
    if (hfsr->filtered_adc == 0.0f) {
        hfsr->filtered_adc = (float)hfsr->raw_adc;
    } else {
        hfsr->filtered_adc = (hfsr->filter_alpha * (float)hfsr->raw_adc) +
                             ((1.0f - hfsr->filter_alpha) * hfsr->filtered_adc);
    }

    // 3. Compute analog voltage
    hfsr->voltage = (hfsr->filtered_adc / FSR_ADC_MAX_16B) * FSR_VREF_VOLTAGE;

    // 4. Schmitt-trigger / Hysteresis State Machine
    FSR_State_t prev_state = hfsr->state;
    if (hfsr->state == FSR_STATE_AIRBORNE) {
        if (hfsr->filtered_adc >= (float)hfsr->contact_threshold) {
            hfsr->state = FSR_STATE_CONTACT;
        }
    } else { // FSR_STATE_CONTACT
        if (hfsr->filtered_adc <= (float)hfsr->release_threshold) {
            hfsr->state = FSR_STATE_AIRBORNE;
        }
    }

    hfsr->state_changed = (hfsr->state != prev_state);
}
