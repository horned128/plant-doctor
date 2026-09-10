/** =================================================================*
 * @file   EnvironmentSensor.c
 * @brief  環境センサー
 * ================================================================= */
#include "EnvironmentSensor.h"                              /* EnvironmentSensorのAPIと型定義 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "I2cBus.h"                                         /* I2CF0共通通信API */
#include "TimeControl.h"                                    /* 同期時間待ちAPI */

#define BME280_ADDRESS_PRIMARY             (0x76U)
#define BME280_ADDRESS_SECONDARY           (0x77U)

#define BME280_REG_CALIB_T_P               (0x88U)
#define BME280_REG_CALIB_H1                (0xA1U)
#define BME280_REG_CHIP_ID                 (0xD0U)
#define BME280_REG_RESET                   (0xE0U)
#define BME280_REG_CALIB_H2_H6             (0xE1U)
#define BME280_REG_CTRL_HUM                (0xF2U)
#define BME280_REG_STATUS                  (0xF3U)
#define BME280_REG_CTRL_MEAS               (0xF4U)
#define BME280_REG_CONFIG                  (0xF5U)
#define BME280_REG_DATA_START              (0xF7U)

#define BME280_CHIP_ID_VALUE               (0x60U)
#define BME280_RESET_COMMAND               (0xB6U)

#define SEN0228_ADDRESS                    (0x10U)
#define SEN0228_CONFIG_REGISTER            (0x00U)
#define SEN0228_ALS_REGISTER               (0x04U)

static bool s_illuminanceConfigured;                         /**< SEN0228初期化完了状態 */
static bool s_bme280Configured;                             /**< BME280初期化完了状態 */
static uint8_t s_bme280Address;                             /**< 検出されたBME280の7bitアドレス */

static uint16_t s_dig_T1;                                   /**< BME280温度補正パラメータT1 */
static int16_t s_dig_T2;                                    /**< BME280温度補正パラメータT2 */
static int16_t s_dig_T3;                                    /**< BME280温度補正パラメータT3 */

static uint16_t s_dig_P1;                                   /**< BME280気圧補正パラメータP1 */
static int16_t s_dig_P2;                                    /**< BME280気圧補正パラメータP2 */
static int16_t s_dig_P3;                                    /**< BME280気圧補正パラメータP3 */
static int16_t s_dig_P4;                                    /**< BME280気圧補正パラメータP4 */
static int16_t s_dig_P5;                                    /**< BME280気圧補正パラメータP5 */
static int16_t s_dig_P6;                                    /**< BME280気圧補正パラメータP6 */
static int16_t s_dig_P7;                                    /**< BME280気圧補正パラメータP7 */
static int16_t s_dig_P8;                                    /**< BME280気圧補正パラメータP8 */
static int16_t s_dig_P9;                                    /**< BME280気圧補正パラメータP9 */

static uint8_t s_dig_H1;                                    /**< BME280湿度補正パラメータH1 */
static int16_t s_dig_H2;                                    /**< BME280湿度補正パラメータH2 */
static uint8_t s_dig_H3;                                    /**< BME280湿度補正パラメータH3 */
static int16_t s_dig_H4;                                    /**< BME280湿度補正パラメータH4 */
static int16_t s_dig_H5;                                    /**< BME280湿度補正パラメータH5 */
static int8_t s_dig_H6;                                     /**< BME280湿度補正パラメータH6 */

/** =================================================================*
 * @brief  BME280のChip IDを確認して存在を判定する。
 * @param[in] address I2Cアドレス
 * @return BME280が存在する場合はtrue
 * ================================================================= */
static bool EnvironmentSensor_ProbeBme280(uint8_t address) {
    const uint8_t reg = BME280_REG_CHIP_ID;
    uint8_t chipId = 0U;

    if (I2cBus_WriteRead(address, &reg, 1U, &chipId, 1U) != I2C_BUS_OK) {
        return false;
    }
    return (chipId == BME280_CHIP_ID_VALUE);
}

/** =================================================================*
 * @brief  BME280のトリミングパラメータを読み取り動作設定を行う。
 * @return 設定成功時はtrue
 * ================================================================= */
static bool EnvironmentSensor_ConfigureBme280(void) {
    uint8_t calib1[24];
    uint8_t calib2[7];
    uint8_t reg;
    const uint8_t cmdHum[2] = {BME280_REG_CTRL_HUM, 0x01U};
    const uint8_t cmdCfg[2] = {BME280_REG_CONFIG, 0xA0U};
    const uint8_t cmdMeas[2] = {BME280_REG_CTRL_MEAS, 0x27U};

    /* 温度・気圧キャリブレーションパラメータ取得 (0x88〜0x9F) */
    reg = BME280_REG_CALIB_T_P;
    if (I2cBus_WriteRead(s_bme280Address, &reg, 1U, calib1, 24U) != I2C_BUS_OK) {
        return false;
    }
    s_dig_T1 = (uint16_t)calib1[0] | ((uint16_t)calib1[1] << 8U);
    s_dig_T2 = (int16_t)((uint16_t)calib1[2] | ((uint16_t)calib1[3] << 8U));
    s_dig_T3 = (int16_t)((uint16_t)calib1[4] | ((uint16_t)calib1[5] << 8U));

    s_dig_P1 = (uint16_t)calib1[6] | ((uint16_t)calib1[7] << 8U);
    s_dig_P2 = (int16_t)((uint16_t)calib1[8] | ((uint16_t)calib1[9] << 8U));
    s_dig_P3 = (int16_t)((uint16_t)calib1[10] | ((uint16_t)calib1[11] << 8U));
    s_dig_P4 = (int16_t)((uint16_t)calib1[12] | ((uint16_t)calib1[13] << 8U));
    s_dig_P5 = (int16_t)((uint16_t)calib1[14] | ((uint16_t)calib1[15] << 8U));
    s_dig_P6 = (int16_t)((uint16_t)calib1[16] | ((uint16_t)calib1[17] << 8U));
    s_dig_P7 = (int16_t)((uint16_t)calib1[18] | ((uint16_t)calib1[19] << 8U));
    s_dig_P8 = (int16_t)((uint16_t)calib1[20] | ((uint16_t)calib1[21] << 8U));
    s_dig_P9 = (int16_t)((uint16_t)calib1[22] | ((uint16_t)calib1[23] << 8U));

    /* 湿度H1取得 (0xA1) */
    reg = BME280_REG_CALIB_H1;
    if (I2cBus_WriteRead(s_bme280Address, &reg, 1U, &s_dig_H1, 1U) != I2C_BUS_OK) {
        return false;
    }

    /* 湿度H2〜H6取得 (0xE1〜0xE7) */
    reg = BME280_REG_CALIB_H2_H6;
    if (I2cBus_WriteRead(s_bme280Address, &reg, 1U, calib2, 7U) != I2C_BUS_OK) {
        return false;
    }
    s_dig_H2 = (int16_t)((uint16_t)calib2[0] | ((uint16_t)calib2[1] << 8U));
    s_dig_H3 = calib2[2];
    s_dig_H4 = (int16_t)(((int16_t)calib2[3] << 4U) | (int16_t)(calib2[4] & 0x0FU));
    s_dig_H5 = (int16_t)(((int16_t)calib2[5] << 4U) | (int16_t)((calib2[4] >> 4U) & 0x0FU));
    s_dig_H6 = (int8_t)calib2[6];

    /* ctrl_hum書き込み (0xF2: osrs_h = x1) */
    if (I2cBus_Write(s_bme280Address, cmdHum, 2U) != I2C_BUS_OK) {
        return false;
    }
    /* config書き込み (0xF5: t_sb = 1000ms, filter = off) */
    if (I2cBus_Write(s_bme280Address, cmdCfg, 2U) != I2C_BUS_OK) {
        return false;
    }
    /* ctrl_meas書き込み (0xF4: osrs_t = x1, osrs_p = x1, mode = Normal) */
    if (I2cBus_Write(s_bme280Address, cmdMeas, 2U) != I2C_BUS_OK) {
        return false;
    }
    return true;
}

/** =================================================================*
 * @brief  BME280から気温・湿度・気圧を取得して補正計算を行う。
 * @param[out] sample 取得値格納先
 * @return 取得成功時はtrue
 * ================================================================= */
static bool EnvironmentSensor_ReadAir(ENVIRONMENT_SENSOR_SAMPLE *sample) {
    const uint8_t reg = BME280_REG_DATA_START;
    uint8_t data[8];
    int32_t adc_P;
    int32_t adc_T;
    int32_t adc_H;
    int32_t var1;
    int32_t var2;
    int32_t t_fine;
    int32_t tempCentiC;
    int32_t v_x1_u32r;
    int32_t p_var1;
    int32_t p_var2;
    uint32_t p;

    if (s_bme280Address == 0U) {
        return false;
    }

    if (I2cBus_WriteRead(s_bme280Address, &reg, 1U, data, 8U) != I2C_BUS_OK) {
        return false;
    }

    adc_P = ((int32_t)data[0] << 12U) | ((int32_t)data[1] << 4U) | ((int32_t)data[2] >> 4U);
    adc_T = ((int32_t)data[3] << 12U) | ((int32_t)data[4] << 4U) | ((int32_t)data[5] >> 4U);
    adc_H = ((int32_t)data[6] << 8U) | (int32_t)data[7];

    /* 未測定値の判定 */
    if ((adc_T == 0x80000L) || (adc_H == 0x8000L)) {
        return false;
    }

    /* 温度補正計算 (32bit整数演算: 0.01℃単位) */
    var1 = ((((adc_T >> 3) - ((int32_t)s_dig_T1 << 1))) * ((int32_t)s_dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)s_dig_T1)) *
        ((adc_T >> 4) - ((int32_t)s_dig_T1))) >> 12) * ((int32_t)s_dig_T3)) >> 14;
    t_fine = var1 + var2;
    tempCentiC = (t_fine * 5 + 128) >> 8;
    sample->airTemperatureCentiC = (int16_t)tempCentiC;

    /* 湿度補正計算 (32bit整数演算: 0.01%単位) */
    v_x1_u32r = t_fine - ((int32_t)76800);
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)s_dig_H4) << 20) -
        (((int32_t)s_dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
        (((((((v_x1_u32r * ((int32_t)s_dig_H6)) >> 10) *
        (((v_x1_u32r * ((int32_t)s_dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
        ((int32_t)2097152)) * ((int32_t)s_dig_H2) + 8192) >> 14));
    v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
        ((int32_t)s_dig_H1)) >> 4);
    if (v_x1_u32r < 0) {
        v_x1_u32r = 0;
    } else if (v_x1_u32r > 419430400) {
        v_x1_u32r = 419430400;
    }
    sample->relativeHumidityCentiPercent =
        (uint16_t)((((uint32_t)(v_x1_u32r >> 12)) * 100UL + 512UL) / 1024UL);

    /* 気圧補正計算 (32bit整数演算: Pa単位) */
    p_var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    p_var2 = (((p_var1 >> 2) * (p_var1 >> 2)) >> 11) * ((int32_t)s_dig_P6);
    p_var2 = p_var2 + ((p_var1 * ((int32_t)s_dig_P5)) << 1);
    p_var2 = (p_var2 >> 2) + (((int32_t)s_dig_P4) << 16);
    p_var1 = (((s_dig_P3 * (((p_var1 >> 2) * (p_var1 >> 2)) >> 13)) >> 3) +
              ((((int32_t)s_dig_P2) * p_var1) >> 1)) >> 18;
    p_var1 = ((((32768 + p_var1)) * ((int32_t)s_dig_P1)) >> 15);
    if (p_var1 == 0) {
        sample->barometricPressurePa = 0UL;
        sample->barometricPressureValid = false;
    } else {
        p = (((uint32_t)(((int32_t)1048576) - adc_P) - (p_var2 >> 12))) * 3125U;
        if (p < 0x80000000UL) {
            p = (p << 1) / ((uint32_t)p_var1);
        } else {
            p = (p / (uint32_t)p_var1) * 2U;
        }
        p_var1 = (((int32_t)s_dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
        p_var2 = (((int32_t)(p >> 2)) * ((int32_t)s_dig_P8)) >> 13;
        p = (uint32_t)((int32_t)p + ((p_var1 + p_var2 + s_dig_P7) >> 4));
        sample->barometricPressurePa = p;
        sample->barometricPressureValid = true;
    }
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
    s_bme280Configured = false;
    s_bme280Address = 0U;

    if (EnvironmentSensor_ProbeBme280(BME280_ADDRESS_PRIMARY)) {
        s_bme280Address = BME280_ADDRESS_PRIMARY;
    } else if (EnvironmentSensor_ProbeBme280(BME280_ADDRESS_SECONDARY)) {
        s_bme280Address = BME280_ADDRESS_SECONDARY;
    }

    if (s_bme280Address != 0U) {
        s_bme280Configured = EnvironmentSensor_ConfigureBme280();
    }
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
    sample->barometricPressurePa = 0UL;
    sample->illuminanceCentiLux = 0UL;
    sample->illuminanceRaw = 0U;
    sample->barometricPressureValid = false;

    /* 未接続だった場合の再プローブ（ホットプラグ対応） */
    if (!s_bme280Configured) {
        if (EnvironmentSensor_ProbeBme280(BME280_ADDRESS_PRIMARY)) {
            s_bme280Address = BME280_ADDRESS_PRIMARY;
            s_bme280Configured = EnvironmentSensor_ConfigureBme280();
        } else if (EnvironmentSensor_ProbeBme280(BME280_ADDRESS_SECONDARY)) {
            s_bme280Address = BME280_ADDRESS_SECONDARY;
            s_bme280Configured = EnvironmentSensor_ConfigureBme280();
        }
    }

    if (s_bme280Configured) {
        sample->airTemperatureValid = EnvironmentSensor_ReadAir(sample);
        if (!sample->airTemperatureValid) {
            /* 読み取り失敗時は再設定フラグをクリア */
            s_bme280Configured = false;
        }
    } else {
        sample->airTemperatureValid = false;
    }

    sample->illuminanceValid = EnvironmentSensor_ReadIlluminance(sample);
    return sample->airTemperatureValid || sample->illuminanceValid;
}
