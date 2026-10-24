/** =================================================================*
 * @file   EbmlI2cTelemetry.c
 * @brief  ATOMS3 Liteゲートウェイ向けI2Cスレーブテレメトリ送信実装
 * ================================================================= */
#include "EbmlI2cTelemetry.h"
#include "PlantDoctorI2cProto.h"
#include "I2cBus.h"
#include "SensorManager.h"
#include "PlantAi.h"
#include "PumpControl.h"
#include <stddef.h>
#include <string.h>

#define I2C_TELEMETRY_INTERVAL_TICKS   50U   /* 500ms周期 */

static uint16_t s_intervalTicks = 0U;
static uint8_t (*s_wateringHandler)(void) = NULL;         /**< ATOMS3 Lite からの給水指示の処理先 */

/** =================================================================*
 * @brief  給水指示の処理先登録
 * @details ATOMS3 Lite からの給水指示は、給水前の値を記録する手動給水の経路で処理する。
 *          未登録なら指示を無視する。
 * @param[in] handler 給水要求関数
 * ================================================================= */
void EbmlI2cTelemetry_SetWateringHandler(uint8_t (*handler)(void)) {
    s_wateringHandler = handler;
}

bool EbmlI2cTelemetry_Init(void) {
    s_intervalTicks = I2C_TELEMETRY_INTERVAL_TICKS;
    return true;
}

void EbmlI2cTelemetry_Process10Ms(void) {
    if (s_intervalTicks > 0U) {
        --s_intervalTicks;
        return;
    }
    s_intervalTicks = I2C_TELEMETRY_INTERVAL_TICKS;

    PLANT_SENSOR_SNAPSHOT snap;
    if (!SensorManager_GetLatest(&snap)) {
        return;
    }

    ebml_i2c_telemetry_pkt_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.magic = EBML_I2C_PKT_MAGIC;
    pkt.version = EBML_I2C_PKT_VERSION;
    pkt.length = (uint8_t)sizeof(ebml_i2c_telemetry_pkt_t);
    pkt.sampleSequence = (uint32_t)snap.sampleSequence;
    pkt.soilMoistureRaw = snap.soilMoistureRaw;
    pkt.leafTemperatureCentiC = snap.leafTemperatureCentiC;
    pkt.airTemperatureCentiC = snap.airTemperatureCentiC;
    pkt.relativeHumidityCentiPercent = snap.relativeHumidityCentiPercent;
    pkt.illuminanceRaw = snap.illuminanceRaw;
    pkt.tankLiquidDetected = snap.tankLiquidDetected ? 1U : 0U;
    pkt.pumpOn = PumpControl_IsOn() ? 1U : 0U;
    PLANT_FEATURE_VECTOR vec;
    PlantAi_GetFeatureVector(&vec);

    pkt.stressScore = PlantAi_GetStressScore();
    pkt.statusCode = (uint8_t)PlantAi_GetStatus();
    pkt.soilTrendCode = (uint8_t)PlantAi_GetSoilTrend();
    pkt.aiTrainCount = (uint16_t)PlantAi_GetSolistTrainCount();
    pkt.aiLossPpm = PlantAi_GetSolistLossPpm();
    pkt.aiPhase = PlantAi_GetSolistPhase();
    /* ダッシュボードは30未満を「健全」と表示する。ルールが異常と判定している間は、従来どおり85を送る */
    pkt.aiAnomalyScore = PlantAi_GetAiAnomalyScore();
    if ((pkt.statusCode != (uint8_t)PLANT_STATUS_HEALTHY) && (pkt.statusCode != (uint8_t)PLANT_STATUS_WARNING) &&
        (pkt.statusCode != (uint8_t)PLANT_STATUS_LEARNING)) {
        pkt.aiAnomalyScore = 85U;
    }
    pkt.leafTempRatePerHour = (int16_t)vec.leafTemperatureRatePerHour;
    pkt.soilMoistureRatePerHour = (int16_t)vec.soilMoistureRatePerHour;
    pkt.checksum = ebml_i2c_calc_checksum(&pkt, (uint8_t)(sizeof(pkt) - sizeof(uint16_t)));

    /* I2Cスレーブ(0x42)へテレメトリを送信 (スレーブ未接続時はNACKで即座に復帰) */
    I2C_BUS_STATUS status = I2cBus_Write(EBML_I2C_SLAVE_ADDR, (const uint8_t *)&pkt, (uint16_t)sizeof(pkt));

    if (status == I2C_BUS_OK) {
        /* 送信成功時、ATOMS3 Liteからの指示(給水トリガー等)があれば読み出し */
        ebml_i2c_cmd_pkt_t cmd;
        if (I2cBus_Read(EBML_I2C_SLAVE_ADDR, (uint8_t *)&cmd, (uint16_t)sizeof(cmd)) == I2C_BUS_OK) {
            if ((cmd.command == 1U) && (s_wateringHandler != NULL)) {
                (void)s_wateringHandler();
            }
        }
    }
}
