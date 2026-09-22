#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

struct __attribute__((__may_alias__)) exCanIdInfo {
    uint32_t id:8;
    uint32_t data:16;
    uint32_t mode:5;
    uint32_t res:3;
};

struct __attribute__((__may_alias__)) mitexCanIdInfo {
    uint32_t id:11;
};

/* Compatibility structures for RobStrideSPC with STM32H7 FDCAN */
typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t DLC;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t DLC;
} CAN_RxHeaderTypeDef;

#define txCanIdEx (*((struct exCanIdInfo*)&(txMsg.ExtId)))
#define rxCanIdEx (*((struct exCanIdInfo*)&(rxMsg.ExtId)))

#define mittxCanIdEx (*((struct mitexCanIdInfo*)&(MITtxMsg.StdId)))
#define mitrxCanIdEx (*((struct mitexCanIdInfo*)&(MITrxMsg.StdId)))

extern CAN_TxHeaderTypeDef txMsg;
extern CAN_RxHeaderTypeDef rxMsg;
extern CAN_TxHeaderTypeDef MITtxMsg;
extern CAN_RxHeaderTypeDef MITrxMsg;

extern uint8_t tx_data[8];
extern uint8_t rx_data[8];

#if defined(HAL_FDCAN_MODULE_ENABLED)
extern FDCAN_HandleTypeDef hfdcan1;
void MX_FDCAN1_Init(void);
#endif

void can_txd(void);
void can_MIT_txd(void);
uint8_t can_receive_msg(uint8_t *buf);
uint8_t can_send_msg(uint8_t *msg, uint32_t len);
uint8_t can_MIT_send_msg(uint8_t *msg, uint32_t len);
void parse_motor_feedback(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */
