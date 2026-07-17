/**************************************************************************
 *  @file     pn532_wb55.c
 *  @author   Yehui from Waveshare (modified for STM32WB55, I2C only)
 *  @license  BSD
 **************************************************************************/

#include "stm32wbxx_hal.h"
#include "main.h"
#include "pn532_stm32f1.h"

#define _I2C_ADDRESS                    0x48
#define _I2C_TIMEOUT                    10

extern I2C_HandleTypeDef hi2c1;

/**************************************************************************
 * Reset and Log
 **************************************************************************/
int PN532_Reset(void) {
    HAL_GPIO_WritePin(PN532_RST_GPIO_Port, PN532_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(PN532_RST_GPIO_Port, PN532_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(PN532_RST_GPIO_Port, PN532_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    return PN532_STATUS_OK;
}

void PN532_Log(const char* log) {
//    printf("%s\r\n", log);
}
/**************************************************************************
 * I2C
 **************************************************************************/
void i2c_read(uint8_t* data, uint16_t count) {
    HAL_I2C_Master_Receive(&hi2c1, _I2C_ADDRESS, data, count, _I2C_TIMEOUT);
}

void i2c_write(uint8_t* data, uint16_t count) {
    HAL_I2C_Master_Transmit(&hi2c1, _I2C_ADDRESS, data, count, _I2C_TIMEOUT);
}

int PN532_I2C_ReadData(uint8_t* data, uint16_t count) {
    uint8_t status[] = {0x00};
    uint8_t frame[count + 1];
    i2c_read(status, sizeof(status));
    if (status[0] != PN532_I2C_READY) {
        return PN532_STATUS_ERROR;
    }
    i2c_read(frame, count + 1);
    for (uint8_t i = 0; i < count; i++) {
        data[i] = frame[i + 1];
    }
    return PN532_STATUS_OK;
}

int PN532_I2C_WriteData(uint8_t* data, uint16_t count) {
    i2c_write(data, count);
    return PN532_STATUS_OK;
}

bool PN532_I2C_WaitReady(uint32_t timeout) {
    uint8_t status[] = {0x00};
    uint32_t tickstart = HAL_GetTick();
    while (HAL_GetTick() - tickstart < timeout) {
        i2c_read(status, sizeof(status));
        if (status[0] == PN532_I2C_READY) {
            return true;
        } else {
        	HAL_Delay(5);
        }
    }
    return false;
}

int PN532_I2C_Wakeup(void) {
    HAL_GPIO_WritePin(PN532_REQ_GPIO_Port, PN532_REQ_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(PN532_REQ_GPIO_Port, PN532_REQ_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(PN532_REQ_GPIO_Port, PN532_REQ_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
    return PN532_STATUS_OK;
}

void PN532_I2C_Init(PN532* pn532) {
    pn532->reset      = PN532_Reset;
    pn532->read_data  = PN532_I2C_ReadData;
    pn532->write_data = PN532_I2C_WriteData;
    pn532->wait_ready = PN532_I2C_WaitReady;
    pn532->wakeup     = PN532_I2C_Wakeup;
    pn532->log        = PN532_Log;

    pn532->wakeup();
}

/**
 * @brief  Pack up to 8 UID bytes into a single uint64_t for easy comparison.
 * @note   NTAG/Mifare UIDs are at most 7-10 bytes; this covers up to 8.
 *         If uid_len > 8, only the first 8 bytes are used.
 * @param  uid      Byte array containing the UID
 * @param  uid_len  Number of valid bytes in uid
 * @return Packed UID as a single 64-bit value
 */
uint64_t uid_to_u64(const uint8_t *uid, int32_t uid_len)
{
    uint64_t packed = 0;

    if (uid_len > 8)
    {
        uid_len = 8;
    }

    for (int32_t i = 0; i < uid_len; i++)
    {
        packed = (packed << 8) | uid[i];
    }

    return packed;
}
/**************************************************************************
 * End: I2C
 **************************************************************************/
