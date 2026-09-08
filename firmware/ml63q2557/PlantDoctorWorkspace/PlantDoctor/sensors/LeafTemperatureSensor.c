/** =================================================================*
 * @file   LeafTemperatureSensor.c
 * @brief  葉温センサー
 * ================================================================= */
#include "LeafTemperatureSensor.h"                          /* LeafTemperatureSensorのAPIと型定義 */
#include <limits.h>                                         /* 標準Cの整数範囲 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "I2cBus.h"                                         /* I2CF0共通通信API */

#define SEN0206_ADDRESS                    (0x5AU)
#define SEN0206_OBJECT_REGISTER            (0x07U)
#define SEN0206_ERROR_FLAG                 (0x8000U)

/** =================================================================*
 * @brief  MLX90614用PECを計算する。
 * @param[in] data 計算対象データ
 * @param[in] size データ長
 * @return PEC値
 * ================================================================= */
static uint8_t LeafTemperatureSensor_Crc8(const uint8_t *data, uint8_t size) {
    uint8_t crc = 0U;
    uint8_t index;
    uint8_t bit;

    for (index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; ++bit) {
            crc = ((crc & 0x80U) != 0U) ?
                (uint8_t)((crc << 1U) ^ 0x07U) :
                (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

/** =================================================================*
 * @brief  LeafTemperatureSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool LeafTemperatureSensor_Init(void) {
    /* I2CF0の初期化はLCD初期化が担当する。 */
    return true;
}

/** =================================================================*
 * @brief  LeafTemperatureSensor_Read処理
 * @param[out] temperatureCentiC 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool LeafTemperatureSensor_Read(int16_t *temperatureCentiC) {
    const uint8_t command = SEN0206_OBJECT_REGISTER;
    uint8_t response[3];
    uint8_t pecInput[5];
    uint16_t raw;
    int32_t converted;

    if (temperatureCentiC == NULL) {
        return false;
    }
    if (I2cBus_WriteRead(SEN0206_ADDRESS, &command, 1U, response, 3U) != I2C_BUS_OK) {
        return false;
    }

    pecInput[0] = (uint8_t)(SEN0206_ADDRESS << 1U);
    pecInput[1] = command;
    pecInput[2] = (uint8_t)((SEN0206_ADDRESS << 1U) | 1U);
    pecInput[3] = response[0];
    pecInput[4] = response[1];
    if (LeafTemperatureSensor_Crc8(pecInput, 5U) != response[2]) {
        return false;
    }

    raw = (uint16_t)response[0] | ((uint16_t)response[1] << 8U);
    if ((raw & SEN0206_ERROR_FLAG) != 0U) {
        return false;
    }
    converted = ((int32_t)raw * 2L) - 27315L;
    if ((converted < INT16_MIN) || (converted > INT16_MAX)) {
        return false;
    }
    *temperatureCentiC = (int16_t)converted;
    return true;
}
