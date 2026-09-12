/** =================================================================*
 * @file   ConsoleUart.c
 * @brief  UARTF1ハードウェア連携API実装 (P2-4)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "ConsoleUart.h"
#include "Console.h"
#include "PlantDoctorConfig.h"
#include "smpl_common.h"
#include "uartf1.h"
#include "uartf_common.h"

/** =================================================================*
 * @brief  ConsoleUart_Init処理
 * @param[in] services コンソールサービス構造体
 * @return 初期化成否
 * ================================================================= */
bool ConsoleUart_Init(const CONSOLE_SERVICES *services) {
    uint16_t uafnmod;

    Console_Init(services);

    /* UARTF1ペリフェラルクロック供給有効化 */
    smpl_enablePeripheral(UAF1_PERI);

    /*
     * ボーレート設定計算根拠:
     * システムクロック = HS PLL 48 MHz (48,000,000 Hz)
     * 目標ボーレート = 115,200 bps
     * 分周比計算式: Baud = SysClock / (16 * brDivisorLatch)
     * brDivisorLatch = 48,000,000 / (16 * 115,200) = 26.041666...
     * 整数丸め値: 26
     * 実ボーレート: 48,000,000 / (16 * 26) = 115,384.6 bps (誤差: +0.16%)
     * 誤差はUART規格許容範囲(±2%〜3%)内に完全に収まるため、クロック調整は無効(UARTF_RMV_DIS)。
     */
    uafnmod = (uint16_t)(UARTF_LG_8BIT |
                         UARTF_STP_1BIT |
                         UARTF_PT_NON |
                         UARTF_FEN_ENA |
                         UARTF_FTL_1BYTE);

    uartf1_init(uafnmod, UARTF_RMV_DIS, PLANT_DOCTOR_CONSOLE_BR_DIVISOR);

    return true;
}

/** =================================================================*
 * @brief  ConsoleUart_Process10Ms処理
 * ================================================================= */
void ConsoleUart_Process10Ms(void) {
    uint8_t rxCount = 0U;
    uint8_t txCount = 0U;

    /* 受信処理: 1 tickあたり最大16 byteまでFIFOから吸い出し */
    while ((rxCount < PLANT_DOCTOR_CONSOLE_MAX_BYTES_PER_TICK) &&
           (uartf1_checkReadReady() == 1)) {
        uint8_t ch = (uint8_t)uartf1_getc();
        Console_PutRxChar((char)ch);
        ++rxCount;
    }

    /* コンソールロジック周期処理 (ダンプ状態機械等) */
    Console_Process10Ms();

    /* 送信処理: 1 tickあたり最大16 byteまでFIFOへ投入 */
    while ((txCount < PLANT_DOCTOR_CONSOLE_MAX_BYTES_PER_TICK) &&
           (uartf1_checkWriteBusy() == 0)) {
        char ch;
        if (!Console_GetTxChar(&ch)) {
            break;
        }
        uartf1_putc((uint8_t)ch);
        ++txCount;
    }
}
