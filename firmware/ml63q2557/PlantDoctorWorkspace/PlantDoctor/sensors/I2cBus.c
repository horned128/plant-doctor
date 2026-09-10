/** =================================================================*
 * @file   I2cBus.c
 * @brief  I2CF0共通通信
 * ================================================================= */
#include "I2cBus.h"                                         /* I2cBusのAPIと型定義 */
#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "PlantDoctorConfig.h"                              /* アプリケーション設定 */
#include "i2cf_common.h"                                    /* I2CFドライバ共通定義 */
#include "irq.h"                                            /* 割り込み制御API */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */
#include "smpl_common.h"                                    /* 共通周辺機器制御API */
#include "wdt.h"                                            /* ウォッチドッグAPI */

#define I2C_STATUS_NACK_MASK               (1UL << 0U)
#define I2C_STATUS_CLEAR_MASK              ((1UL << 7U) | (1UL << 1U))
#define I2C_STATUS_BUS_BUSY_MASK           (1UL << 5U)
#define I2C_STATUS_COMPLETE_MASK           (1UL << 7U)

#define I2C_CONTROL_REPEAT_MASK            (1UL << 2U)
#define I2C_CONTROL_TX_NACK_MASK           (1UL << 3U)
#define I2C_CONTROL_TRANSMIT_MASK          (1UL << 4U)
#define I2C_CONTROL_MASTER_MASK            (1UL << 5U)
#define I2C_CONTROL_CLOCK_HOLD_MASK        (1UL << 13U)

#define I2C_P72_RESET_DISABLE_MASK         (1UL << 2U)
#define I2C_P73_SCL_MASK                   (1UL << 3U)
#define I2C_P74_SDA_MASK                   (1UL << 4U)
#define I2C_P75_BACKLIGHT_MASK             (1UL << 5U)

#define I2C_P7MOD0_SCL_I2C                 (0x2BUL << 24U)
#define I2C_P7MOD0_SCL_GPIO_OUT            (0x02UL << 24U)
#define I2C_P7MOD0_SCL_MASK                (0xFFUL << 24U)
#define I2C_P7MOD0_RESET_GPIO_OUT          (0x02UL << 16U)
#define I2C_P7MOD0_RESET_MASK              (0xFFUL << 16U)

#define I2C_P7MOD1_SDA_I2C                 (0x2BUL << 0U)
#define I2C_P7MOD1_SDA_GPIO_IN_PULLUP      (0x01UL << 0U)
#define I2C_P7MOD1_SDA_GPIO_OUT            (0x02UL << 0U)
#define I2C_P7MOD1_SDA_MASK                (0xFFUL << 0U)
#define I2C_P7MOD1_BACKLIGHT_GPIO_OUT      (0x02UL << 8U)
#define I2C_P7MOD1_BACKLIGHT_MASK          (0xFFUL << 8U)

#define I2C_RATE_STANDARD_100KHZ           (0x3CU)

/** =================================================================*
 * @brief  数マイクロ秒単位の微小遅延を生成する。
 * ================================================================= */
static void I2cBus_DelayShort(void) {
    volatile uint32_t count = 60UL;

    while (count > 0UL) {
        --count;
    }
}

/** =================================================================*
 * @brief  転送完了フラグをクリアする。
 * ================================================================= */
static void I2cBus_ClearComplete(void) {
    clear_bit(I2CF0->I2F0SR, I2C_STATUS_CLEAR_MASK);
}

/** =================================================================*
 * @brief  I2Cバスのスタック状態を検出し、クロック送出で解放する。
 * @return バスが解放された場合はtrue
 * ================================================================= */
bool I2cBus_Recover(void) {
    uint8_t pulse;
    bool recovered = false;

    /* P73(SCL)をHigh出力、P74(SDA)をプルアップ入力へ一時設定 */
    set_bit(PORT7->P7DO, I2C_P73_SCL_MASK);
    write_bit(PORT7->P7MOD0, I2C_P7MOD0_SCL_MASK, I2C_P7MOD0_SCL_GPIO_OUT);
    write_bit(PORT7->P7MOD1, I2C_P7MOD1_SDA_MASK, I2C_P7MOD1_SDA_GPIO_IN_PULLUP);
    I2cBus_DelayShort();

    /* SDAがHighであればバスはスタックしていない */
    if ((read_reg32(PORT7->P7DI) & I2C_P74_SDA_MASK) != 0UL) {
        recovered = true;
    } else {
        /* スレーブがSDAをLow固定している場合、SCLへ最大9クロック出力して解放を促す */
        for (pulse = 0U; pulse < 9U; ++pulse) {
            clear_bit(PORT7->P7DO, I2C_P73_SCL_MASK);
            I2cBus_DelayShort();
            set_bit(PORT7->P7DO, I2C_P73_SCL_MASK);
            I2cBus_DelayShort();
            if ((read_reg32(PORT7->P7DI) & I2C_P74_SDA_MASK) != 0UL) {
                recovered = true;
                break;
            }
        }
    }

    /* STOP条件をソフトウェア生成してバスを確実にアイドル化 */
    clear_bit(PORT7->P7DO, I2C_P73_SCL_MASK);
    clear_bit(PORT7->P7DO, I2C_P74_SDA_MASK);
    write_bit(PORT7->P7MOD1, I2C_P7MOD1_SDA_MASK, I2C_P7MOD1_SDA_GPIO_OUT);
    I2cBus_DelayShort();
    set_bit(PORT7->P7DO, I2C_P73_SCL_MASK);
    I2cBus_DelayShort();
    set_bit(PORT7->P7DO, I2C_P74_SDA_MASK);
    I2cBus_DelayShort();
    write_bit(PORT7->P7MOD1, I2C_P7MOD1_SDA_MASK, I2C_P7MOD1_SDA_GPIO_IN_PULLUP);
    I2cBus_DelayShort();

    return recovered;
}

/** =================================================================*
 * @brief  I2CF0ペリフェラルおよび関連GPIO端子を初期化する。
 * @return 初期化成功時はtrue
 * ================================================================= */
bool I2cBus_Init(void) {
    uint32_t interruptState = __get_PRIMASK();

    __disable_irq();
    irq_i2cf0_dis();
    smpl_enablePeripheral(I2CF0_PERI);

    /* LCDリセット端子(P72)を出力High(リセット解除)に設定 */
    set_bit(PORT7->P7DO, I2C_P72_RESET_DISABLE_MASK);
    write_bit(PORT7->P7MOD0, I2C_P7MOD0_RESET_MASK, I2C_P7MOD0_RESET_GPIO_OUT);

    /* LCDバックライト端子(P75)を出力Low(消灯)に設定 */
    clear_bit(PORT7->P7DO, I2C_P75_BACKLIGHT_MASK);
    write_bit(PORT7->P7MOD1, I2C_P7MOD1_BACKLIGHT_MASK, I2C_P7MOD1_BACKLIGHT_GPIO_OUT);

    /* I2Cバス解放（スタックリカバリ）の実行 */
    (void)I2cBus_Recover();

    /* P73(SCL)およびP74(SDA)をI2CF0機能端子に設定 */
    write_bit(PORT7->P7MOD0, I2C_P7MOD0_SCL_MASK, I2C_P7MOD0_SCL_I2C);
    write_bit(PORT7->P7MOD1, I2C_P7MOD1_SDA_MASK, I2C_P7MOD1_SDA_I2C);

    /* I2CF0ハードウェアを標準100kHzモードで初期化 */
    clear_bit(I2CF0->I2F0CTL, (1UL << 7U));
    write_reg32(I2CF0->I2F0BC, I2C_RATE_STANDARD_100KHZ);
    write_reg32(I2CF0->I2F0CTL, ((uint32_t)I2F_MOD_STD | (1UL << 7U)));
    clear_bit(I2CF0->I2F0MOD, (1UL << 0U));
    clear_bit(I2CF0->I2F0CTL, (1UL << 12U));
    clear_bit(I2CF0->I2F0CTL, (1UL << 11U));
    set_bit(I2CF0->I2F0CTL, (1UL << 9U));
    clear_bit(I2CF0->I2F0CTL, (1UL << 6U));

    irq_i2cf0_clearIRQ();
    I2cBus_ClearComplete();

    if (interruptState == 0U) {
        __enable_irq();
    }
    return true;
}

/** =================================================================*
 * @brief  指定したI2C状態ビットを待機する。
 * @param[in] mask 状態ビットマスク
 * @param[in] set 待機する状態
 * @return 指定状態になった場合はtrue、タイムアウト時はfalse
 * ================================================================= */
static bool I2cBus_WaitStatus(uint32_t mask, bool set) {
    uint32_t remaining = PLANT_DOCTOR_I2C_TIMEOUT_LOOPS;

    while (remaining > 0UL) {
        if (get_bit(I2CF0->I2F0SR, mask) == set) {
            return true;
        }
        --remaining;
        if ((remaining & 0x3FFFUL) == 0UL) {
            wdt_clear();
        }
    }
    return false;
}


/** =================================================================*
 * @brief  I2CバスへSTOP条件を送出し、バス解放を確認する。
 * @return バス解放確認結果
 * ================================================================= */
static I2C_BUS_STATUS I2cBus_Stop(void) {
    uint32_t control = read_reg32(I2CF0->I2F0CTL);

    control &= ~(I2C_CONTROL_MASTER_MASK | I2C_CONTROL_CLOCK_HOLD_MASK |
        I2C_CONTROL_REPEAT_MASK | I2C_CONTROL_TX_NACK_MASK);
    write_reg32(I2CF0->I2F0CTL, control);
    return I2cBus_WaitStatus(I2C_STATUS_BUS_BUSY_MASK, false) ? I2C_BUS_OK : I2C_BUS_BUSY;
}

/** =================================================================*
 * @brief  送信完了とACKを確認する。
 * @return 転送結果
 * ================================================================= */
static I2C_BUS_STATUS I2cBus_WaitTransmitComplete(void) {
    if (!I2cBus_WaitStatus(I2C_STATUS_COMPLETE_MASK, true)) {
        return I2C_BUS_TIMEOUT;
    }
    if (get_bit(I2CF0->I2F0SR, I2C_STATUS_NACK_MASK)) {
        I2cBus_ClearComplete();
        return I2C_BUS_NACK;
    }
    I2cBus_ClearComplete();
    return I2C_BUS_OK;
}

/** =================================================================*
 * @brief  スレーブアドレスを送信して転送を開始する。
 * @param[in] address7Bit 7ビットスレーブアドレス
 * @param[in] receive 受信転送の場合はtrue
 * @return 転送結果
 * ================================================================= */
static I2C_BUS_STATUS I2cBus_Begin(uint8_t address7Bit, bool receive) {
    if (!I2cBus_WaitStatus(I2C_STATUS_BUS_BUSY_MASK, false)) {
        return I2C_BUS_BUSY;
    }

    clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_CLOCK_HOLD_MASK | I2C_CONTROL_REPEAT_MASK | I2C_CONTROL_TX_NACK_MASK);
    if (receive) {
        clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_TRANSMIT_MASK);
    } else {
        set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TRANSMIT_MASK);
    }

    write_reg32(I2CF0->I2F0DR, ((uint32_t)address7Bit << 1U) | (receive ? 1UL : 0UL));
    set_bit(I2CF0->I2F0CTL, I2C_CONTROL_MASTER_MASK);
    return I2cBus_WaitTransmitComplete();
}

/** =================================================================*
 * @brief  アドレス送信済みの受信転送を完了する。
 * @param[out] data 受信バッファ
 * @param[in] size 受信バイト数
 * @return 転送結果
 * ================================================================= */
static I2C_BUS_STATUS I2cBus_ReceiveAfterAddress(uint8_t *data, uint16_t size) {
    uint16_t index;

    if (size == 1U) {
        set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
    }
    (void)read_reg32(I2CF0->I2F0DR);

    for (index = 0U; index < size; ++index) {
        if (!I2cBus_WaitStatus(I2C_STATUS_COMPLETE_MASK, true)) {
            (void)I2cBus_Stop();
            return I2C_BUS_TIMEOUT;
        }
        if ((size > 1U) && (index == (uint16_t)(size - 2U))) {
            set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
        }
        if (index == (uint16_t)(size - 1U)) {
            clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_MASTER_MASK);
            clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
        }
        data[index] = (uint8_t)read_reg32(I2CF0->I2F0DR);
        I2cBus_ClearComplete();
    }

    return I2cBus_Stop();
}

/** =================================================================*
 * @brief  I2Cスレーブへデータを書き込む。
 * @param[in] address7Bit 7ビットスレーブアドレス
 * @param[in] data 送信バッファ
 * @param[in] size 送信バイト数
 * @return 転送結果
 * ================================================================= */
I2C_BUS_STATUS I2cBus_Write(uint8_t address7Bit, const uint8_t *data, uint16_t size) {
    I2C_BUS_STATUS status;
    I2C_BUS_STATUS stopStatus;
    uint16_t index;

    if ((address7Bit > 0x7FU) || (data == NULL) || (size == 0U)) {
        return I2C_BUS_INVALID_ARGUMENT;
    }

    status = I2cBus_Begin(address7Bit, false);
    for (index = 0U; (index < size) && (status == I2C_BUS_OK); ++index) {
        write_reg32(I2CF0->I2F0DR, data[index]);
        status = I2cBus_WaitTransmitComplete();
    }
    stopStatus = I2cBus_Stop();
    if (status == I2C_BUS_OK) {
        status = stopStatus;
    }
    return status;
}

/** =================================================================*
 * @brief  I2Cスレーブからデータを読み出す。
 * @param[in] address7Bit 7ビットスレーブアドレス
 * @param[out] data 受信バッファ
 * @param[in] size 受信バイト数
 * @return 転送結果
 * ================================================================= */
I2C_BUS_STATUS I2cBus_Read(uint8_t address7Bit, uint8_t *data, uint16_t size) {
    I2C_BUS_STATUS status;

    if ((address7Bit > 0x7FU) || (data == NULL) || (size == 0U)) {
        return I2C_BUS_INVALID_ARGUMENT;
    }

    status = I2cBus_Begin(address7Bit, true);
    if (status != I2C_BUS_OK) {
        (void)I2cBus_Stop();
        return status;
    }
    return I2cBus_ReceiveAfterAddress(data, size);
}

/** =================================================================*
 * @brief  書き込み後にリピートスタートで読み出す。
 * @param[in] address7Bit 7ビットスレーブアドレス
 * @param[in] writeData 送信バッファ
 * @param[in] writeSize 送信バイト数
 * @param[out] readData 受信バッファ
 * @param[in] readSize 受信バイト数
 * @return 転送結果
 * ================================================================= */
I2C_BUS_STATUS I2cBus_WriteRead(uint8_t address7Bit, const uint8_t *writeData,
    uint16_t writeSize, uint8_t *readData, uint16_t readSize) {
    I2C_BUS_STATUS status;
    uint16_t index;
    uint32_t control;

    if ((address7Bit > 0x7FU) || (writeData == NULL) || (writeSize == 0U) || (readData == NULL) || (readSize == 0U)) {
        return I2C_BUS_INVALID_ARGUMENT;
    }

    status = I2cBus_Begin(address7Bit, false);
    for (index = 0U; (index < writeSize) && (status == I2C_BUS_OK); ++index) {
        if (index == (uint16_t)(writeSize - 1U)) {
            set_bit(I2CF0->I2F0CTL, I2C_CONTROL_CLOCK_HOLD_MASK);
        }
        write_reg32(I2CF0->I2F0DR, writeData[index]);
        status = I2cBus_WaitTransmitComplete();
    }
    if (status != I2C_BUS_OK) {
        (void)I2cBus_Stop();
        return status;
    }

    control = read_reg32(I2CF0->I2F0CTL);
    control |= I2C_CONTROL_REPEAT_MASK;
    control &= ~(I2C_CONTROL_TRANSMIT_MASK | I2C_CONTROL_CLOCK_HOLD_MASK);
    write_reg32(I2CF0->I2F0CTL, control);
    write_reg32(I2CF0->I2F0DR, ((uint32_t)address7Bit << 1U) | 1UL);

    status = I2cBus_WaitTransmitComplete();
    if (status != I2C_BUS_OK) {
        (void)I2cBus_Stop();
        return status;
    }
    return I2cBus_ReceiveAfterAddress(readData, readSize);
}
