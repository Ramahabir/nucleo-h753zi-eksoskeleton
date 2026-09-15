#include "bno085.h"
#include <stdio.h>

void BNO085_Init(BNO085_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *hintn_port, uint16_t hintn_pin){
    dev->hspi       = hspi;
    dev->cs_port    = cs_port;
    dev->cs_pin     = cs_pin;
    dev->rst_port   = rst_port;
    dev->rst_pin    = rst_pin;
    dev->hintn_port = hintn_port;
    dev->hintn_pin  = hintn_pin;

    // Deselect chip and ensure reset is high
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_SET);
}

void BNO085_HardwareReset(BNO085_t *dev){
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(15);
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_SET);
}

bool BNO085_WaitHINTN(BNO085_t *dev, uint32_t timeout_ms){
    uint32_t deadline = HAL_GetTick() + timeout_ms;
    while (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET){
        if (HAL_GetTick() > deadline){
            return false;
        }
    }
    return true;
}

HAL_StatusTypeDef BNO085_ReadHeader(BNO085_t *dev, SHTP_Header_t *header){
    uint8_t tx_dummy[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t rx_raw[4] = {0x00, 0x00, 0x00, 0x00};

    // Assert CS
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);

    // Clock out 4 dummy to read the SHTP Header
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(dev->hspi, tx_dummy, rx_raw, 4, 100);

    // Deassert CS
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK){
        return status;
    }

    // Parse SHTP Header
    header->length              = (uint16_t)rx_raw[0] | (((uint16_t)(rx_raw[1] & 0x7F)) << 8);
    header->has_continuation    = (rx_raw[1] & 0x80) != 0;
    header->channel             = rx_raw[2];
    header->sequence            = rx_raw[3];

    return HAL_OK;
}

HAL_StatusTypeDef BNO085_Stage1_Test(BNO085_t *dev)
{
    printf("\r\n========================================\r\n");
    printf("   BNO085 Stage 1: SPI Proof-of-Life\r\n");
    printf("========================================\r\n");
    /* 1. Pulse Hardware Reset */
    printf("[1/3] Resetting BNO085...\r\n");
    BNO085_HardwareReset(dev);
    /* 2. Wait for HINTN assertion */
    printf("[2/3] Waiting for HINTN falling edge...\r\n");
    if (!BNO085_WaitHINTN(dev, 1000))
    {
        printf("[FAIL] HINTN timeout! Check PS0/PS1 (both must be 3.3V) and RST wiring.\r\n");
        return HAL_TIMEOUT;
    }
    printf("[OK] HINTN asserted LOW! Sensor is ready.\r\n");
    /* 3. Read SHTP Header */
    printf("[3/3] Reading 4-Byte SHTP Header via SPI2...\r\n");
    SHTP_Header_t hdr = {0};
    HAL_StatusTypeDef status = BNO085_ReadHeader(dev, &hdr);
    if (status != HAL_OK)
    {
        printf("[FAIL] SPI transfer error: %d\r\n", status);
        return status;
    }
    printf("----------------------------------------\r\n");
    printf("Packet Length : %u bytes\r\n", hdr.length);
    printf("Channel       : %u\r\n", hdr.channel);
    printf("Sequence No   : %u\r\n", hdr.sequence);
    printf("Continuation  : %s\r\n", hdr.has_continuation ? "Yes" : "No");
    printf("----------------------------------------\r\n");
    if (hdr.length > 4 && (hdr.channel == SHTP_CHAN_COMMAND || hdr.channel == SHTP_CHAN_CONTROL))
    {
        printf(">>> SUCCESS: Valid BNO085 advertisement received! SPI2 is operational! <<<\r\n");
        return HAL_OK;
    }
    else
    {
        printf("[WARNING] Header invalid or empty. Verify SPI Mode 3 (CPOL=1, CPHA=1) and MISO pin.\r\n");
        return HAL_ERROR;
    }
}


uint16_t BNO085_ReadPacket(BNO085_t *dev, uint8_t *buffer, uint16_t buffer_size) {
    if (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        return 0; // HINTN is HIGH, no incoming packet
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 200; i++) {}

    // 1. Read 4-byte SHTP Header using zero dummy bytes on MOSI
    uint8_t tx_zero[4] = {0, 0, 0, 0};
    uint8_t header[4] = {0};
    if (HAL_SPI_TransmitReceive(dev->hspi, tx_zero, header, 4, 100) != HAL_OK) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
        return 0;
    }

    uint16_t packet_len = (header[0] | (header[1] << 8)) & 0x7FFF;
    if (packet_len < 4 || packet_len == 0x7FFF) {
        HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
        return 0;
    }

    buffer[0] = header[0];
    buffer[1] = header[1];
    buffer[2] = header[2];
    buffer[3] = header[3];

    // 2. Read remainder of the packet with zeroes on MOSI
    uint16_t remaining = packet_len - 4;
    uint16_t to_read = (remaining < (buffer_size - 4)) ? remaining : (buffer_size - 4);

    if (to_read > 0) {
        static uint8_t tx_zeros[300] = {0};
        HAL_SPI_TransmitReceive(dev->hspi, tx_zeros, &buffer[4], to_read, 100);
    }

    // 3. Drain excess if packet > buffer_size
    if (remaining > to_read) {
        uint8_t z = 0, d = 0;
        for (uint16_t i = 0; i < (remaining - to_read); i++) {
            HAL_SPI_TransmitReceive(dev->hspi, &z, &d, 1, 10);
        }
    }

    for (volatile int i = 0; i < 200; i++) {}
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
    return packet_len;
}

bool BNO085_SendPacket(BNO085_t *dev, uint8_t *tx_buf, uint16_t len) {
    /*
     * SHTP SPI is FULL DUPLEX.  The host can only write to the sensor
     * when HINTN is LOW (sensor has data ready).  During that CS-low
     * window both sides exchange their cargo simultaneously:
     *   bytes [0..3]  = SHTP headers (host→sensor AND sensor→host)
     *   bytes [4..N]  = payloads, length = max(host_payload, sensor_payload)
     * The shorter side pads with 0x00.
     */

    /* ---- 1. Wait for HINTN LOW (sensor ready) ---- */
    uint32_t start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->hintn_port, dev->hintn_pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - start) > 500) {
            printf("  [FAIL] HINTN timeout – sensor asleep, cannot send.\r\n");
            return false;
        }
    }

    /* ---- 2. CS LOW ---- */
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 200; i++) {}

    /* ---- 3. Exchange 4-byte SHTP headers (full-duplex) ---- */
    uint8_t rx_hdr[4] = {0};
    HAL_SPI_TransmitReceive(dev->hspi, tx_buf, rx_hdr, 4, 100);

    /* Parse sensor's header so we know how long its cargo is */
    uint16_t sensor_pkt_len = (rx_hdr[0] | (rx_hdr[1] << 8)) & 0x7FFF;
    uint16_t sensor_payload = (sensor_pkt_len > 4) ? (sensor_pkt_len - 4) : 0;
    uint16_t host_payload   = (len > 4) ? (len - 4) : 0;
    uint16_t xfer_len       = (sensor_payload > host_payload) ? sensor_payload : host_payload;

    /* ---- 4. Exchange payloads (full-duplex) ---- */
    if (xfer_len > 0) {
        /* Build host TX buffer: our payload padded with zeros */
        static uint8_t tx_pad[300] = {0};
        static uint8_t rx_pad[300] = {0};
        for (uint16_t i = 0; i < 300; i++) tx_pad[i] = 0;
        if (host_payload > 0 && host_payload <= 296) {
            for (uint16_t i = 0; i < host_payload; i++) tx_pad[i] = tx_buf[4 + i];
        }
        uint16_t safe_len = (xfer_len > 296) ? 296 : xfer_len;
        HAL_SPI_TransmitReceive(dev->hspi, tx_pad, rx_pad, safe_len, 200);
    }

    /* ---- 5. CS HIGH ---- */
    for (volatile int i = 0; i < 200; i++) {}
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    printf("  [TX OK] Sent %u bytes on CH%u. Sensor simultaneously sent %u bytes on CH%u.\r\n",
           len, tx_buf[2], sensor_pkt_len, rx_hdr[2]);

    return true;
}


void BNO085_EnableGameRotationVector(BNO085_t *dev) {
    uint8_t tx_buf[21] = {0};
    
    // 1. SHTP Header (4 bytes)
    tx_buf[0] = 21;    // Packet Length LSB
    tx_buf[1] = 0;     // Packet Length MSB
    tx_buf[2] = 2;     // Channel 2 (SH-2 Control)
    tx_buf[3] = 0;     // Sequence Number
    
    // 2. Set Feature Command (17 bytes)
    tx_buf[4] = 0xFD;  // Report ID: Set Feature Command
    tx_buf[5] = 0x08;  // Feature Report ID: Game Rotation Vector
    tx_buf[6] = 0x00;  // Feature Flags
    tx_buf[7] = 0x00;  // Change Sensitivity LSB
    tx_buf[8] = 0x00;  // Change Sensitivity MSB
    
    // Report Interval (5000 us = 0x00001388 = 200 Hz)
    tx_buf[9]  = 0x88; 
    tx_buf[10] = 0x13;
    tx_buf[11] = 0x00;
    tx_buf[12] = 0x00;
    
    // Batch Interval (0 = no batching)
    tx_buf[13] = 0x00;
    tx_buf[14] = 0x00;
    tx_buf[15] = 0x00;
    tx_buf[16] = 0x00;
    
    // Sensor-specific configuration (0 for Game Rotation Vector)
    tx_buf[17] = 0x00;
    tx_buf[18] = 0x00;
    tx_buf[19] = 0x00;
    tx_buf[20] = 0x00;

    printf("[CMD] Sending Set Feature Command (200Hz)...\r\n");
    if (BNO085_SendPacket(dev, tx_buf, 21)) {
        printf("[OK] Set Feature Command Transmitted!\r\n");
    } else {
        printf("[FAIL] Set Feature Command SPI error!\r\n");
    }
}

void BNO085_Stage2_PollData(BNO085_t *dev) {
    uint8_t packet[300];
    uint16_t len = BNO085_ReadPacket(dev, packet, sizeof(packet));
    if (len == 0) {
        return;
    }

    uint8_t channel = packet[2];

    // Sensor-Hub Input Channel (reports)
    if (channel == 3) {
        // Scan for Game Rotation Vector Report ID (0x08)
        for (int i = 4; i <= (len - 14); i++) {
            if (packet[i] == 0x08) {
                int16_t q_i = (int16_t)(packet[i+4] | (packet[i+5] << 8));
                int16_t q_j = (int16_t)(packet[i+6] | (packet[i+7] << 8));
                int16_t q_k = (int16_t)(packet[i+8] | (packet[i+9] << 8));
                int16_t q_r = (int16_t)(packet[i+10] | (packet[i+11] << 8));
                
                float fi = q_i / 16384.0f;
                float fj = q_j / 16384.0f;
                float fk = q_k / 16384.0f;
                float fr = q_r / 16384.0f;
                
                printf("Quat [r, i, j, k]: %+.3f, %+.3f, %+.3f, %+.3f\r\n", fr, fi, fj, fk);
                return;
            }
        }
        printf("[SensorHub CH3] %u bytes, first ID: 0x%02X\r\n", len, packet[4]);
    } else {
        printf("[Incoming CH%u] %u bytes received (Report 0x%02X)\r\n", channel, len, packet[4]);
    }
}



