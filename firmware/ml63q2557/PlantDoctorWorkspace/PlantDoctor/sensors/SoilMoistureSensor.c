/** =================================================================*
 * @file   SoilMoistureSensor.c
 * @brief  土壌水分センサー
 * ================================================================= */
#include "SoilMoistureSensor.h"                             /* SoilMoistureSensorのAPIと型定義 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "irq.h"                                            /* 割り込み制御API */
#include "saAdc0.h"                                         /* ADC0 API */
#include "smpl_common.h"                                    /* 共通周辺機器制御API */
#include "wdt.h"                                            /* ウォッチドッグAPI */

#define ADC_TIMEOUT_LOOPS                  (480000UL)

/** =================================================================*
 * @brief  SoilMoistureSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SoilMoistureSensor_Init(void) {
    initAdc_t adcConfig = {0};
    enableAdcChannel_t channels = {0};

    smpl_enablePeripheral(SAD0_PERI);
    irq_sad0_dis();
    irq_sad0_clearIRQ();
    adcConfig.discharge = SAADC_SAINIT_DISCHARGE;
    adcConfig.holdTime = 0x03U;
    adcConfig.clock = SAADC_SACK_OSCLK_DIV16;
    adcConfig.mode = SAADC_SALP_ONESHOT;
    adcConfig.limitInterrupt = SAADC_SALMD_INSIDE_LIMIT;
    adcConfig.limitMode = SAADC_SALEN_DISABLE;
    adcConfig.ampStabilityTime = 0x02U;
    adcConfig.interruptMode = SAADC_SADIMD0_ALL_CH;
    adcConfig.interruptLimitMode = SAADC_SADIMD1_LIMIT_MATCH;
    adcConfig.interval = 0U;
    adcConfig.channelSync = SAADC_SYNC_NORMAL;
    saAdc0_init(&adcConfig);
    channels.ch0 = SAADC_RUN;
    saAdc0_setEnableChannel(&channels);
    return true;
}

/** =================================================================*
 * @brief  SoilMoistureSensor_Read処理
 * @param[out] rawValue 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool SoilMoistureSensor_Read(uint16_t *rawValue) {
    uint32_t remaining = ADC_TIMEOUT_LOOPS;
    uint16_t adcRaw;

    if (rawValue == NULL) {
        return false;
    }
    saAdc0_start();
    while ((saAdc0_getRunning() != 0U) && (remaining > 0UL)) {
        --remaining;
        if ((remaining & 0x3FFFUL) == 0UL) {
            wdt_clear();
        }
    }
    if (remaining == 0UL) {
        saAdc0_stop();
        return false;
    }

    adcRaw = (uint16_t)((saAdc0_getResult0() & 0xFFF0UL) >> 4U);
    /* CN6の反転増幅回路に合わせ、0が乾燥側、4095が湿潤側となるよう反転する。 */
    *rawValue = (uint16_t)(4095U - adcRaw);
    return true;
}
