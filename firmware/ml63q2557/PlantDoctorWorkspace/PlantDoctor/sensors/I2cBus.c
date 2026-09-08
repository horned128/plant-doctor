/** =================================================================*
 * @file   I2cBus.c
 * @brief  I2CF0共通通信
 * ================================================================= */
#include "I2cBus.h"                                         /* I2cBusのAPIと型定義 */
#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stddef.h>                                         /* 標準CのNULL定義 */
#include "PlantDoctorConfig.h"                              /* アプリケーション設定 */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */
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

/** =================================================================*
 * @brief  指定したI2C状態ビットを待機する。
 * @param[in] mask 状態ビットマスク
 * @param[in] set 待機する状態
 * @return 指定状態になった場合はtrue、タイムアウト時はfalse
 * ================================================================= */
static bool I2cBus_WaitStatus(uint32_t mask, bool set) {
    uint32_t remaining = PLANT_DOCTOR_LCD_TIMEOUT_LOOPS;

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
 * @brief  転送完了フラグをクリアする。
 * ================================================================= */
static void I2cBus_ClearComplete(void) {
    clear_bit(I2CF0->I2F0SR, I2C_STATUS_CLEAR_MASK);
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
