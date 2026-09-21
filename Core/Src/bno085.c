#include "bno085.h"
#include <math.h>
#include <string.h>

/* Small delay helper for SPI CS setup and hold timing */
static inline void BNO085_Delay_Short(void) {
    for (volatile int i = 0; i < 50; i++) {
        __NOP();
    }
}

BNO085_Status_t BNO085_Init(BNO085_t *dev, SPI_HandleTypeDef *hspi,
                            GPIO_TypeDef *cs_port,    uint16_t cs_pin,
                            GPIO_TypeDef *rst_port,   uint16_t rst_pin,
                            GPIO_TypeDef *hintn_port, uint16_t hintn_pin,
                            GPIO_TypeDef *wake_port,  uint16_t wake_pin) {
    if (!dev || !hspi || !cs_port || !rst_port || !hintn_port || !wake_port) {
        return BNO085_ERR_PARAM;
    }

    dev->hspi       = hspi;
    dev->cs_port    = cs_port;
    dev->cs_pin     = cs_pin;
    dev->rst_port   = rst_port;
    dev->rst_pin    = rst_pin;
    dev->hintn_port = hintn_port;
    dev->hintn_pin  = hintn_pin;
    dev->wake_port  = wake_port;
    dev->wake_pin   = wake_pin;

    memset(&dev->data, 0, sizeof(dev->data));
    memset(dev->sequence_number, 0, sizeof(dev->sequence_number));

    /* Initialize control line states: CS high (unselected), RST high (run), WAKE high (SPI mode) */
    HAL_GPIO_WritePin(dev->cs_port,   dev->cs_pin,   GPIO_PIN_SET);
    HAL_GPIO_WritePin(dev->rst_port,  dev->rst_pin,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(dev->wake_port, dev->wake_pin, GPIO_PIN_SET);

    return BNO085_OK;
}

BNO085_Status_t BNO085_HardwareReset(BNO085_t *dev, uint32_t timeout_ms) {
    if (!dev) return BNO085_ERR_PARAM;

    /* 1. Pulse hardware reset line LOW for 15 ms */
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(15);
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_SET);

    /* 2. Wait for HINTN to assert LOW (sensor finished booting) */
    uint32_t start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            return BNO085_ERR_TIMEOUT;
        }
    }

    /* 3. Drain initial advertisement / reset report to clear HINTN */
    uint8_t boot_buf[128];
    BNO085_ReadPacket(dev, boot_buf, sizeof(boot_buf));

    return BNO085_OK;
}

bool BNO085_HasData(BNO085_t *dev) {
    if (!dev) return false;
    return (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_RESET);
}

uint16_t BNO085_ReadPacket(BNO085_t *dev, uint8_t *buffer, uint16_t buffer_size) {
    if (!dev || !buffer || buffer_size < 4) return 0;

    /* Check if sensor has data ready (HINTN active LOW) */
    if (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        return 0;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    BNO085_Delay_Short();

    /* 1. Read 4-byte SHTP Header */
    uint8_t tx_zero[4] = {0, 0, 0, 0};
    uint8_t header[4] = {0};
    if (HAL_SPI_TransmitReceive(dev->hspi, tx_zero, header, 4, 50) != HAL_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
        return 0;
    }

    uint16_t packet_len = (header[0] | ((uint16_t)(header[1] & 0x7F) << 8));
    if (packet_len < 4 || packet_len == 0x7FFF) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
        return 0;
    }

    buffer[0] = header[0];
    buffer[1] = header[1];
    buffer[2] = header[2];
    buffer[3] = header[3];

    /* 2. Read payload */
    uint16_t remaining = packet_len - 4;
    uint16_t to_read = (remaining < (buffer_size - 4)) ? remaining : (buffer_size - 4);

    if (to_read > 0) {
        /* Read payload in 32-byte stack chunks with dummy zeros on MOSI */
        uint8_t tx_dummy[32] = {0};
        uint16_t transferred = 0;
        while (transferred < to_read) {
            uint16_t chunk = (to_read - transferred > 32) ? 32 : (to_read - transferred);
            if (HAL_SPI_TransmitReceive(dev->hspi, tx_dummy, &buffer[4 + transferred], chunk, 50) != HAL_OK) {
                break;
            }
            transferred += chunk;
        }
    }

    /* 3. Drain any remaining unread bytes from SPI if buffer was smaller than packet */
    if (remaining > to_read) {
        uint8_t z = 0, d = 0;
        for (uint16_t i = 0; i < (remaining - to_read); i++) {
            HAL_SPI_TransmitReceive(dev->hspi, &z, &d, 1, 5);
        }
    }

    BNO085_Delay_Short();
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    return (to_read + 4 <= packet_len) ? (to_read + 4) : packet_len;
}

BNO085_Status_t BNO085_SendPacket(BNO085_t *dev, uint8_t channel, const uint8_t *payload, uint16_t payload_len) {
    if (!dev || (!payload && payload_len > 0)) return BNO085_ERR_PARAM;

    /* 1. Assert WAKE LOW to notify sensor host wants to write */
    HAL_GPIO_WritePin(dev->wake_port, dev->wake_pin, GPIO_PIN_RESET);

    /* 2. Wait for sensor to acknowledge WAKE by asserting HINTN LOW */
    uint32_t start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - start) > 200) {
            HAL_GPIO_WritePin(dev->wake_port, dev->wake_pin, GPIO_PIN_SET);
            return BNO085_ERR_TIMEOUT;
        }
    }

    /* 3. Assert CS LOW */
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    BNO085_Delay_Short();

    /* 4. Deassert WAKE HIGH per CEVA reference manual */
    HAL_GPIO_WritePin(dev->wake_port, dev->wake_pin, GPIO_PIN_SET);

    /* 5. Transfer SHTP Header */
    uint16_t total_len = payload_len + 4;
    uint8_t tx_hdr[4];
    tx_hdr[0] = (uint8_t)(total_len & 0xFF);
    tx_hdr[1] = (uint8_t)((total_len >> 8) & 0x7F);
    tx_hdr[2] = channel;
    tx_hdr[3] = (channel < 6) ? dev->sequence_number[channel]++ : 0;

    uint8_t rx_hdr[4] = {0};
    if (HAL_SPI_TransmitReceive(dev->hspi, tx_hdr, rx_hdr, 4, 50) != HAL_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
        return BNO085_ERR_SPI;
    }

    /* Check full-duplex length requirements */
    uint16_t sensor_len = (rx_hdr[0] | ((uint16_t)(rx_hdr[1] & 0x7F) << 8));
    uint16_t sensor_payload = (sensor_len > 4) ? (sensor_len - 4) : 0;
    uint16_t host_payload   = payload_len;
    uint16_t xfer_len       = (sensor_payload > host_payload) ? sensor_payload : host_payload;

    /* 6. Transfer payload in stack chunks */
    if (xfer_len > 0) {
        uint8_t tx_chunk[32];
        uint8_t rx_chunk[32];
        uint16_t transferred = 0;
        while (transferred < xfer_len) {
            uint16_t chunk = (xfer_len - transferred > 32) ? 32 : (xfer_len - transferred);
            for (uint16_t i = 0; i < chunk; i++) {
                uint16_t idx = transferred + i;
                tx_chunk[i] = (idx < host_payload) ? payload[idx] : 0x00;
            }
            if (HAL_SPI_TransmitReceive(dev->hspi, tx_chunk, rx_chunk, chunk, 50) != HAL_OK) {
                HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
                return BNO085_ERR_SPI;
            }
            transferred += chunk;
        }
    }

    BNO085_Delay_Short();
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    /* 7. Wait for HINTN to return HIGH (bus released) */
    start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - start) > 50) break;
    }

    return BNO085_OK;
}

BNO085_Status_t BNO085_EnableGameRotationVector(BNO085_t *dev, uint16_t report_interval_ms) {
    if (!dev) return BNO085_ERR_PARAM;

    uint32_t interval_us = (uint32_t)report_interval_ms * 1000;
    uint8_t cmd[17] = {0};

    cmd[0] = SHTP_REPORT_SET_FEATURE;          /* 0xFD: Set Feature Command */
    cmd[1] = SHTP_REPORT_GAME_ROTATION_VECTOR; /* 0x08: Game Rotation Vector */
    cmd[2] = 0x00;                             /* Feature Flags */
    cmd[3] = 0x00;                             /* Change Sensitivity LSB */
    cmd[4] = 0x00;                             /* Change Sensitivity MSB */

    /* Report Interval in microseconds (32-bit unsigned little-endian) */
    cmd[5] = (uint8_t)(interval_us & 0xFF);
    cmd[6] = (uint8_t)((interval_us >> 8) & 0xFF);
    cmd[7] = (uint8_t)((interval_us >> 16) & 0xFF);
    cmd[8] = (uint8_t)((interval_us >> 24) & 0xFF);

    /* Batch Interval = 0 (no batching) */
    cmd[9]  = 0x00;
    cmd[10] = 0x00;
    cmd[11] = 0x00;
    cmd[12] = 0x00;

    /* Sensor-specific configuration word = 0 */
    cmd[13] = 0x00;
    cmd[14] = 0x00;
    cmd[15] = 0x00;
    cmd[16] = 0x00;

    return BNO085_SendPacket(dev, SHTP_CHAN_SH2_CONFIG, cmd, sizeof(cmd));
}

BNO085_Status_t BNO085_Update(BNO085_t *dev) {
    if (!dev) return BNO085_ERR_PARAM;

    /* Fast check: If HINTN is HIGH, sensor has no data ready */
    if (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        return BNO085_NO_DATA;
    }

    uint8_t packet[128];
    uint16_t len = BNO085_ReadPacket(dev, packet, sizeof(packet));
    if (len < 4) {
        return BNO085_NO_DATA;
    }

    uint8_t channel = packet[2];

    /* Channel 3: Sensor Input Reports */
    if (channel == SHTP_CHAN_SH2_INPUT) {
        uint16_t offset = 4;
        while (offset < len) {
            uint8_t report_id = packet[offset];

            /* Timestamp reference report is 5 bytes: ID (0xFB) + 4 bytes tick */
            if (report_id == SHTP_REPORT_TIMESTAMP_BASE) {
                offset += 5;
                continue;
            }

            /* Rotation vector reports */
            if (report_id == SHTP_REPORT_GAME_ROTATION_VECTOR ||
                report_id == SHTP_REPORT_ROTATION_VECTOR ||
                report_id == SHTP_REPORT_ARVR_STABILIZED_GRV) {

                if ((offset + 12) <= len) {
                    int16_t q_i = (int16_t)(packet[offset + 4]  | (packet[offset + 5]  << 8));
                    int16_t q_j = (int16_t)(packet[offset + 6]  | (packet[offset + 7]  << 8));
                    int16_t q_k = (int16_t)(packet[offset + 8]  | (packet[offset + 9]  << 8));
                    int16_t q_r = (int16_t)(packet[offset + 10] | (packet[offset + 11] << 8));

                    /* Scale Q14 fixed-point to float [-1.0f, +1.0f] */
                    dev->data.q_i = q_i * (1.0f / 16384.0f);
                    dev->data.q_j = q_j * (1.0f / 16384.0f);
                    dev->data.q_k = q_k * (1.0f / 16384.0f);
                    dev->data.q_r = q_r * (1.0f / 16384.0f);

                    dev->data.sequence = packet[offset + 1];
                    dev->data.accuracy = packet[offset + 2] & 0x03;

                    /* Compute Euler angles (Yaw, Pitch, Roll in degrees) */
                    float siny_cosp = 2.0f * (dev->data.q_r * dev->data.q_k + dev->data.q_i * dev->data.q_j);
                    float cosy_cosp = 1.0f - 2.0f * (dev->data.q_j * dev->data.q_j + dev->data.q_k * dev->data.q_k);
                    dev->data.yaw   = 57.2957795f * atan2f(siny_cosp, cosy_cosp);

                    float sinp = 2.0f * (dev->data.q_r * dev->data.q_j - dev->data.q_k * dev->data.q_i);
                    if (fabsf(sinp) >= 1.0f) {
                        dev->data.pitch = (sinp >= 0.0f) ? 90.0f : -90.0f;
                    } else {
                        dev->data.pitch = 57.2957795f * asinf(sinp);
                    }

                    float sinr_cosp = 2.0f * (dev->data.q_r * dev->data.q_i + dev->data.q_j * dev->data.q_k);
                    float cosr_cosp = 1.0f - 2.0f * (dev->data.q_i * dev->data.q_i + dev->data.q_j * dev->data.q_j);
                    dev->data.roll  = 57.2957795f * atan2f(sinr_cosp, cosr_cosp);

                    dev->data.last_update_tick = HAL_GetTick();
                    dev->data.has_new_data = true;

                    uint16_t report_size = (report_id == SHTP_REPORT_ROTATION_VECTOR) ? 14 : 12;
                    offset += report_size;
                    continue;
                }
            }

            /* Advance past unhandled report or break */
            break;
        }
        return BNO085_OK;
    }

    return BNO085_OK;
}
