/** =================================================================*
 * @file   EnvironmentSensor.c
 * @brief  環境センサー
 * ================================================================= */
#include "EnvironmentSensor.h"                              /* EnvironmentSensorのAPIと型定義 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "I2cBus.h"                                         /* I2CF0共通通信API */
#include "TimeControl.h"                                    /* 同期時間待ちAPI */

#define SEN0385_ADDRESS                    (0x44U)
#define SEN0385_MEASURE_MSB                (0x24U)
#define SEN0385_MEASURE_LSB                (0x00U)

#define SEN0228_ADDRESS                    (0x10U)
#define SEN0228_CONFIG_REGISTER            (0x00U)
#define SEN0228_ALS_REGISTER               (0x04U)

static bool s_illuminanceConfigured;                       /**< SEN0228初期化完了状態 */

/** =================================================================*
 * @brief  SHT31用CRC-8を計算する。
 * @param[in] data 計算対象データ
 * @param[in] size データ長
 * @return CRC-8値
 * ================================================================= */
static uint8_t EnvironmentSensor_Crc8(const uint8_t *data, uint8_t size) {
    uint8_t crc = 0xFFU;
    uint8_t index;
    uint8_t bit;

    for (index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; ++bit) {
            crc = ((crc & 0x80U) != 0U) ?
                (uint8_t)((crc << 1U) ^ 0x31U) :
                (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

/** =================================================================*
 * @brief  SEN0385から気温と相対湿度を取得する。
 * @param[out] sample 取得値格納先
 * @return 取得成功時はtrue
 * ================================================================= */
static bool EnvironmentSensor_ReadAir(ENVIRONMENT_SENSOR_SAMPLE *sample) {
    const uint8_t command[2] = {SEN0385_MEASURE_MSB, SEN0385_MEASURE_LSB};
    uint8_t response[6];
    uint16_t temperatureRaw;
    uint16_t humidityRaw;

    if (I2cBus_Write(SEN0385_ADDRESS, command, 2U) != I2C_BUS_OK) {
        return false;
    }
    if (!TimeControlDelayMs(20U)) {
        return false;
    }
    if (I2cBus_Read(SEN0385_ADDRESS, response, 6U) != I2C_BUS_OK) {
        return false;
    }
    if ((EnvironmentSensor_Crc8(response, 2U) != response[2]) ||
        (EnvironmentSensor_Crc8(&response[3], 2U) != response[5])) {
        return false;
    }

    temperatureRaw = ((uint16_t)response[0] << 8U) | response[1];
    humidityRaw = ((uint16_t)response[3] << 8U) | response[4];
    sample->airTemperatureCentiC = (int16_t)(-4500L + (((int32_t)17500L * temperatureRaw + 32767L) / 65535L));
    sample->relativeHumidityCentiPercent = (uint16_t)(((uint32_t)10000UL * humidityRaw + 32767UL) / 65535UL);
    return true;
}

/** =================================================================*
 * @brief  SEN0228から照度を取得する。
 * @param[out] sample 取得値格納先
 * @return 取得成功時はtrue
 * ================================================================= */
static bool EnvironmentSensor_ReadIlluminance(ENVIRONMENT_SENSOR_SAMPLE *sample) {
    const uint8_t config[3] = {SEN0228_CONFIG_REGISTER, 0x00U, 0x00U};
    const uint8_t alsRegister = SEN0228_ALS_REGISTER;
    uint8_t response[2];

    if (!s_illuminanceConfigured) {
        if (I2cBus_Write(SEN0228_ADDRESS, config, 3U) != I2C_BUS_OK) {
            return false;
        }
        if (!TimeControlDelayMs(120U)) {
            return false;
        }
        s_illuminanceConfigured = true;
    }
    if (I2cBus_WriteRead(SEN0228_ADDRESS, &alsRegister, 1U, response, 2U) != I2C_BUS_OK) {
        return false;
    }

    sample->illuminanceRaw = (uint16_t)response[0] | ((uint16_t)response[1] << 8U);
    sample->illuminanceCentiLux = (((uint32_t)sample->illuminanceRaw * 576UL) + 50UL) / 100UL;
    return true;
}

/** =================================================================*
 * @brief  EnvironmentSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool EnvironmentSensor_Init(void) {
    /* I2CF0の初期化はLCD初期化が担当する。センサー未接続は正常系として扱う。 */
    s_illuminanceConfigured = false;
    return true;
}

/** =================================================================*
 * @brief  EnvironmentSensor_Read処理
 * @param[out] sample 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool EnvironmentSensor_Read(ENVIRONMENT_SENSOR_SAMPLE *sample) {
    if (sample == NULL) {
        return false;
    }

    sample->airTemperatureCentiC = 0;
    sample->relativeHumidityCentiPercent = 0U;
    sample->illuminanceCentiLux = 0UL;
    sample->illuminanceRaw = 0U;
    sample->airTemperatureValid = EnvironmentSensor_ReadAir(sample);
    sample->illuminanceValid = EnvironmentSensor_ReadIlluminance(sample);
    return sample->airTemperatureValid || sample->illuminanceValid;
}
