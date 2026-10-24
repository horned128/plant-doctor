/** =================================================================*
 * @file   EbmlI2cTelemetry.h
 * @brief  ATOMS3 Liteゲートウェイ向けI2Cスレーブテレメトリ送信モジュール
 * ================================================================= */
#ifndef EBML_I2C_TELEMETRY_H
#define EBML_I2C_TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>

bool EbmlI2cTelemetry_Init(void);
void EbmlI2cTelemetry_Process10Ms(void);
void EbmlI2cTelemetry_SetWateringHandler(uint8_t (*handler)(void)); /* ATOMS3 Lite からの給水指示の処理先 */

#endif /* EBML_I2C_TELEMETRY_H */
