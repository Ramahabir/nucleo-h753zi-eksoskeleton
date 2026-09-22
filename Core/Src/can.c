#include "can.h"
#include <string.h>

CAN_TxHeaderTypeDef txMsg = {0};
CAN_RxHeaderTypeDef rxMsg = {0};

CAN_TxHeaderTypeDef MITtxMsg = {0};
CAN_RxHeaderTypeDef MITrxMsg = {0};

uint8_t tx_data[8] = {0};
uint8_t rx_data[8] = {0};

#if defined(HAL_FDCAN_MODULE_ENABLED)
FDCAN_HandleTypeDef hfdcan1;
#endif

uint8_t can_send_msg(uint8_t *msg, uint32_t len) {
#if defined(HAL_FDCAN_MODULE_ENABLED)
    FDCAN_TxHeaderTypeDef fdcan_tx = {0};
    fdcan_tx.Identifier          = txMsg.ExtId;
    fdcan_tx.IdType              = FDCAN_EXTENDED_ID;
    fdcan_tx.TxFrameType         = FDCAN_DATA_FRAME;
    fdcan_tx.DataLength          = FDCAN_DLC_BYTES_8;
    fdcan_tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    fdcan_tx.BitRateSwitch       = FDCAN_BRS_OFF;
    fdcan_tx.FDFormat            = FDCAN_CLASSIC_CAN;
    fdcan_tx.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    fdcan_tx.MessageMarker       = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &fdcan_tx, msg) != HAL_OK) {
        return 1;
    }
    return 0;
#else
    return 1;
#endif
}

uint8_t can_MIT_send_msg(uint8_t *msg, uint32_t len) {
#if defined(HAL_FDCAN_MODULE_ENABLED)
    FDCAN_TxHeaderTypeDef fdcan_tx = {0};
    fdcan_tx.Identifier          = MITtxMsg.StdId;
    fdcan_tx.IdType              = FDCAN_STANDARD_ID;
    fdcan_tx.TxFrameType         = FDCAN_DATA_FRAME;
    fdcan_tx.DataLength          = FDCAN_DLC_BYTES_8;
    fdcan_tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    fdcan_tx.BitRateSwitch       = FDCAN_BRS_OFF;
    fdcan_tx.FDFormat            = FDCAN_CLASSIC_CAN;
    fdcan_tx.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    fdcan_tx.MessageMarker       = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &fdcan_tx, msg) != HAL_OK) {
        return 1;
    }
    return 0;
#else
    return 1;
#endif
}

uint8_t can_receive_msg(uint8_t *buf) {
#if defined(HAL_FDCAN_MODULE_ENABLED)
    FDCAN_RxHeaderTypeDef fdcan_rx = {0};
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &fdcan_rx, buf) != HAL_OK) {
        return 0;
    }
    if (fdcan_rx.IdType == FDCAN_EXTENDED_ID) {
        rxMsg.ExtId = fdcan_rx.Identifier;
        rxMsg.StdId = 0;
    } else {
        rxMsg.StdId = fdcan_rx.Identifier;
        rxMsg.ExtId = 0;
    }
    rxMsg.DLC = 8;
    return 8;
#else
    return 0;
#endif
}

void can_txd(void) {
    can_send_msg(tx_data, 8);
}

void can_MIT_txd(void) {
    can_MIT_send_msg(tx_data, 8);
}

