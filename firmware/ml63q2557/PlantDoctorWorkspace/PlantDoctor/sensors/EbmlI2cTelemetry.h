/** =================================================================*
 * @file   EbmlI2cTelemetry.h
 * @brief  ATOMS3 Liteゲートウェイ向けI2Cスレーブテレメトリ送信モジュール
 * ================================================================= */
#ifndef EBML_I2C_TELEMETRY_H
#define EBML_I2C_TELEMETRY_H

#include <stdbool.h>

bool EbmlI2cTelemetry_Init(void);
void EbmlI2cTelemetry_Process10Ms(void);

#endif /* EBML_I2C_TELEMETRY_H */
