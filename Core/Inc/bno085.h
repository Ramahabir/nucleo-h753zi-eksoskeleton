#ifndef __BNO085_H
#define __BNO085_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Driver status & error codes
 */
typedef enum {
    BNO085_OK             = 0,
    BNO085_NO_DATA        = 1,   /* HINTN pin is HIGH (no data available) */
    BNO085_ERR_TIMEOUT    = -1,  /* Timeout waiting for HINTN / hardware */
    BNO085_ERR_SPI        = -2,  /* HAL SPI transfer error */
    BNO085_ERR_HEADER     = -3,  /* Corrupt or invalid SHTP header */
    BNO085_ERR_PARAM      = -4   /* Invalid function argument */
} BNO085_Status_t;

/* SHTP Logic Channels */
#define SHTP_CHAN_COMMAND       0
#define SHTP_CHAN_CONTROL       1
#define SHTP_CHAN_SH2_CONFIG    2
#define SHTP_CHAN_SH2_INPUT     3

/* SHTP Command & Report IDs */
#define SHTP_REPORT_COMMAND_RESPONSE     0xF1
#define SHTP_REPORT_COMMAND_REQUEST      0xF2
#define SHTP_REPORT_FRS_READ_RESPONSE    0xF3
#define SHTP_REPORT_FRS_READ_REQUEST     0xF4
#define SHTP_REPORT_PRODUCT_ID_RESPONSE  0xF8
#define SHTP_REPORT_PRODUCT_ID_REQUEST   0xF9
#define SHTP_REPORT_TIMESTAMP_BASE       0xFB
#define SHTP_REPORT_SET_FEATURE          0xFD

/* Sensor Feature Report IDs */
#define SHTP_REPORT_ROTATION_VECTOR         0x05
#define SHTP_REPORT_GAME_ROTATION_VECTOR    0x08
#define SHTP_REPORT_ARVR_STABILIZED_GRV     0x28

/**
 * @brief Measurement Data Structure
 */
typedef struct {
    /* Quaternions (Normalized: -1.0f to +1.0f) */
    float q_r;  /* Real (W) */
    float q_i;  /* X */
    float q_j;  /* Y */
    float q_k;  /* Z */

    /* Euler angles in degrees (Derived from quaternion) */
    float yaw;
    float pitch;
    float roll;

    uint8_t  accuracy;           /* 0: Unreliable, 1: Low, 2: Medium, 3: High */
    uint8_t  sequence;           /* SHTP sensor report sequence number */
    uint32_t last_update_tick;   /* HAL_GetTick() when data was received */
    bool     has_new_data;       /* Set to true when fresh data arrived */
} BNO085_Data_t;

/**
 * @brief BNO085 Device Instance Handle
 */
typedef struct {
    SPI_HandleTypeDef   *hspi;
    GPIO_TypeDef        *cs_port;
    uint16_t            cs_pin;
    GPIO_TypeDef        *rst_port;
    uint16_t            rst_pin;
    GPIO_TypeDef        *hintn_port;
    uint16_t            hintn_pin;
    GPIO_TypeDef        *wake_port;
    uint16_t            wake_pin;

    /* SHTP sequence counters for channels 0..5 */
    uint8_t             sequence_number[6];

    /* Output measurement data */
    BNO085_Data_t       data;
} BNO085_t;

/**
 * @brief SHTP 4-Byte Header
 */
typedef struct {
    uint16_t length;
    uint8_t  channel;
    uint8_t  sequence;
    bool     has_continuation;
} SHTP_Header_t;

/* Public Driver API */

/**
 * @brief Initialize device handle and control pin states
 */
BNO085_Status_t BNO085_Init(BNO085_t *dev, SPI_HandleTypeDef *hspi,
                            GPIO_TypeDef *cs_port,    uint16_t cs_pin,
                            GPIO_TypeDef *rst_port,   uint16_t rst_pin,
                            GPIO_TypeDef *hintn_port, uint16_t hintn_pin,
                            GPIO_TypeDef *wake_port,  uint16_t wake_pin);

/**
 * @brief Pulse hardware reset and wait for initial boot packet
 */
BNO085_Status_t BNO085_HardwareReset(BNO085_t *dev, uint32_t timeout_ms);

/**
 * @brief Non-blocking check if sensor has asserted HINTN (data ready)
 */
bool BNO085_HasData(BNO085_t *dev);

/**
 * @brief Enable Game Rotation Vector at a specified report interval
 * @param report_interval_ms Update period in milliseconds (e.g., 10 = 100 Hz, 20 = 50 Hz)
 */
BNO085_Status_t BNO085_EnableGameRotationVector(BNO085_t *dev, uint16_t report_interval_ms);

/**
 * @brief Non-blocking poll & update function.
 *        Call periodically in control loop or from EXTI ISR context.
 * @return BNO085_OK if packet was read and parsed,
 *         BNO085_NO_DATA if no packet was waiting (0 delay),
 *         or an error code.
 */
BNO085_Status_t BNO085_Update(BNO085_t *dev);

/**
 * @brief Send an SHTP packet to the sensor (re-entrant, thread-safe)
 */
BNO085_Status_t BNO085_SendPacket(BNO085_t *dev, uint8_t channel, const uint8_t *payload, uint16_t payload_len);

/**
 * @brief Read a raw SHTP packet from the sensor
 */
uint16_t BNO085_ReadPacket(BNO085_t *dev, uint8_t *buffer, uint16_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* __BNO085_H */
