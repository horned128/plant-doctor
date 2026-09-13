/** =================================================================*
 * @file   ebml_i2c_proto.h
 * @brief  DT-EBMLとATOMS3 Lite間のI2C通信プロトコル定義
 * ================================================================= */
#ifndef EBML_I2C_PROTO_H
#define EBML_I2C_PROTO_H

#include <stdint.h>

#define EBML_I2C_SLAVE_ADDR        0x42
#define EBML_I2C_PKT_MAGIC         0x5044   /* 'P', 'D' */
#define EBML_I2C_PKT_VERSION       2

#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                     /* 0x5044 */
    uint8_t  version;                   /* 2 */
    uint8_t  length;                    /* sizeof(ebml_i2c_telemetry_pkt_t) */
    uint32_t sampleSequence;            /* sample count */
    uint16_t soilMoistureRaw;           /* 0-4095 */
    int16_t  leafTemperatureCentiC;     /* centi-Celsius */
    int16_t  airTemperatureCentiC;      /* centi-Celsius */
    uint16_t relativeHumidityCentiPercent; /* centi-percent */
    uint16_t illuminanceRaw;            /* lux / raw */
    uint8_t  tankLiquidDetected;        /* 1 or 0 */
    uint8_t  pumpOn;                    /* 1 or 0 */
    uint8_t  stressScore;               /* 0-100 */
    uint8_t  statusCode;                /* 0: HEALTHY, 1: DRY_STRESS, etc. */
    uint8_t  soilTrendCode;             /* 0: UNKNOWN, 1: STABLE, etc. */
    
    /* Solist-AI™ 特化テレメトリ */
    uint16_t aiTrainCount;              /* オンデバイス学習ステップ累積数 */
    uint16_t aiLossPpm;                 /* 最新再構成損失 (0.0001〜1.0000 を 1〜10000 で表現) */
    uint8_t  aiPhase;                   /* 0:Profiling, 1:Stabilizing, 2:Monitoring */
    uint8_t  aiAnomalyScore;            /* 0-100 異常度スコア */
    int16_t  leafTempRatePerHour;       /* 葉温変化率 [1/100 ℃/h] */
    int16_t  soilMoistureRatePerHour;   /* 土壌水分変化率 [‰/h] */

    uint16_t checksum;                  /* 16-bit additive checksum of bytes 0..length-3 */
} ebml_i2c_telemetry_pkt_t;

typedef struct {
    uint8_t  command;                   /* 0: NOP, 1: WATERING */
    uint8_t  reserved;
} ebml_i2c_cmd_pkt_t;
#pragma pack(pop)

static inline uint16_t ebml_i2c_calc_checksum(const void *buf, uint8_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += (uint16_t)p[i];
    }
    return sum;
}

#endif /* EBML_I2C_PROTO_H */
