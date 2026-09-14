#ifndef __BNO085_H
#define __BNO085_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

// SHTP Logic Channels
#define SHTP_CHAN_COMMAND       0
#define SHTP_CHAN_CONTROL       1
#define SHTP_CHAN_SH2_CONFIG    2
#define SHTP_CHAN_SH2_INPUT     3

// BNO085 Device Handle Structure
typedef struct {
    SPI_HandleTypeDef   *hspi;
    GPIO_TypeDef        *cs_port;
    uint16_t            cs_pin;
    GPIO_TypeDef        *rst_port;
    uint16_t            rst_pin;
    GPIO_TypeDef        *hintn_port;
    uint16_t            hintn_pin;
} BNO085_t;

// SHTP 4-Byte Header Presentation
typedef struct {
    uint16_t length;
    uint16_t channel;
    uint16_t sequence;
    bool has_continuation;
} SHTP_Header_t;

// Public API
void BNO085_Init(BNO085_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *hintn_port, uint16_t hintn_pin);

void BNO085_HardwareReset(BNO085_t *dev);

bool BNO085_WaitHINTN(BNO085_t *dev, uint32_t timeout_ms);
HAL_StatusTypeDef BNO085_ReadHeader(BNO085_t *dev, SHTP_Header_t *header);

HAL_StatusTypeDef BNO085_Stage1_Test(BNO085_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __BNO085_H*/
