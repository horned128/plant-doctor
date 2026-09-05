/** =================================================================*
 * @file   DiagnosticI2c.c
 * @brief  診断用I2C通信
 * ================================================================= */
#include "DiagnosticI2c.h"                                  /* DiagnosticI2cのAPIと型定義 */
#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stddef.h>                                         /* 標準Cのサイズ型 */
#include "PlantDoctorConfig.h"                              /* PlantDoctorConfigのAPIと型定義 */
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
 * @brief  DiagnosticI2c_WaitStatus処理
 * @param[in] mask 引数
 * @param[in] set 引数
 * @return 実行結果または取得値
 * ================================================================= */
static bool DiagnosticI2c_WaitStatus(uint32_t mask, bool set) {
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
 * @brief  DiagnosticI2c_ClearComplete処理
 * @return 実行結果または取得値
 * ================================================================= */
static void DiagnosticI2c_ClearComplete(void) {
    clear_bit(I2CF0->I2F0SR, I2C_STATUS_CLEAR_MASK);
}

/** =================================================================*
 * @brief  DiagnosticI2c_Stop処理
 * @return 実行結果または取得値
 * ================================================================= */
static void DiagnosticI2c_Stop(void) {
    uint32_t control = read_reg32(I2CF0->I2F0CTL);

    control &= ~(I2C_CONTROL_MASTER_MASK |
        I2C_CONTROL_CLOCK_HOLD_MASK |
        I2C_CONTROL_REPEAT_MASK |
        I2C_CONTROL_TX_NACK_MASK);
    write_reg32(I2CF0->I2F0CTL, control);
}

/** =================================================================*
 * @brief  DiagnosticI2c_WaitTransmitComplete処理
 * @return 実行結果または取得値
 * ================================================================= */
static DIAGNOSTIC_I2C_STATUS DiagnosticI2c_WaitTransmitComplete(void) {
    if (!DiagnosticI2c_WaitStatus(I2C_STATUS_COMPLETE_MASK, true)) {
        return DIAGNOSTIC_I2C_TIMEOUT;
    }
    if (get_bit(I2CF0->I2F0SR, I2C_STATUS_NACK_MASK)) {
        DiagnosticI2c_ClearComplete();
        return DIAGNOSTIC_I2C_NACK;
    }
    DiagnosticI2c_ClearComplete();
    return DIAGNOSTIC_I2C_OK;
}

/** =================================================================*
 * @brief  DiagnosticI2c_Begin処理
 * @param[in] address7Bit 引数
 * @param[in] receive 引数
 * @return 実行結果または取得値
 * ================================================================= */
static DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Begin(
    uint8_t address7Bit,
    bool receive) {
    if (!DiagnosticI2c_WaitStatus(I2C_STATUS_BUS_BUSY_MASK, false)) {
        return DIAGNOSTIC_I2C_BUS_BUSY;
    }

    clear_bit(I2CF0->I2F0CTL,
        I2C_CONTROL_CLOCK_HOLD_MASK |
        I2C_CONTROL_REPEAT_MASK |
        I2C_CONTROL_TX_NACK_MASK);
    if (receive) {
        clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_TRANSMIT_MASK);
    } else {
        set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TRANSMIT_MASK);
    }

    write_reg32(I2CF0->I2F0DR,
        ((uint32_t)address7Bit << 1U) | (receive ? 1UL : 0UL));
    set_bit(I2CF0->I2F0CTL, I2C_CONTROL_MASTER_MASK);
    return DiagnosticI2c_WaitTransmitComplete();
}

/** =================================================================*
 * @brief  DiagnosticI2c_ReceiveAfterAddress処理
 * @param[out] data 引数
 * @param[in] size 引数
 * @return 実行結果または取得値
 * ================================================================= */
static DIAGNOSTIC_I2C_STATUS DiagnosticI2c_ReceiveAfterAddress(
    uint8_t *data,
    uint16_t size) {
    uint16_t index;

    if (size == 1U) {
        set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
    }

    /* スレーブアドレス直後の最初の読み出しはI2CFのダミー読み出しである。 */
    (void)read_reg32(I2CF0->I2F0DR);

    for (index = 0U; index < size; ++index) {
        if (!DiagnosticI2c_WaitStatus(I2C_STATUS_COMPLETE_MASK, true)) {
            DiagnosticI2c_Stop();
            return DIAGNOSTIC_I2C_TIMEOUT;
        }

        if ((size > 1U) && (index == (uint16_t)(size - 2U))) {
            set_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
        }
        if (index == (uint16_t)(size - 1U)) {
            clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_MASTER_MASK);
            clear_bit(I2CF0->I2F0CTL, I2C_CONTROL_TX_NACK_MASK);
        }

        data[index] = (uint8_t)read_reg32(I2CF0->I2F0DR);
        /* 受信時はI2F0DRを読み出してからMCFを消去する。 */
        DiagnosticI2c_ClearComplete();
    }

    DiagnosticI2c_Stop();
    return DIAGNOSTIC_I2C_OK;
}

/** =================================================================*
 * @brief  DiagnosticI2c_Probe処理
 * @param[in] address7Bit 引数
 * @return 実行結果または取得値
 * ================================================================= */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Probe(uint8_t address7Bit) {
    DIAGNOSTIC_I2C_STATUS status;

    if (address7Bit > 0x7FU) {
        return DIAGNOSTIC_I2C_INVALID_ARGUMENT;
    }
    status = DiagnosticI2c_Begin(address7Bit, false);
    DiagnosticI2c_Stop();
    return status;
}

/** =================================================================*
 * @brief  DiagnosticI2c_Write処理
 * @param[in] address7Bit 引数
 * @param[in] data 引数
 * @param[in] size 引数
 * @return 実行結果または取得値
 * ================================================================= */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Write(
    uint8_t address7Bit,
    const uint8_t *data,
    uint16_t size) {
    DIAGNOSTIC_I2C_STATUS status;
    uint16_t index;

    if ((address7Bit > 0x7FU) || (data == NULL) || (size == 0U)) {
        return DIAGNOSTIC_I2C_INVALID_ARGUMENT;
    }

    status = DiagnosticI2c_Begin(address7Bit, false);
    for (index = 0U; (index < size) && (status == DIAGNOSTIC_I2C_OK); ++index) {
        write_reg32(I2CF0->I2F0DR, data[index]);
        status = DiagnosticI2c_WaitTransmitComplete();
    }
    DiagnosticI2c_Stop();
    return status;
}

/** =================================================================*
 * @brief  DiagnosticI2c_Read処理
 * @param[in] address7Bit 引数
 * @param[out] data 引数
 * @param[in] size 引数
 * @return 実行結果または取得値
 * ================================================================= */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Read(
    uint8_t address7Bit,
    uint8_t *data,
    uint16_t size) {
    DIAGNOSTIC_I2C_STATUS status;

    if ((address7Bit > 0x7FU) || (data == NULL) || (size == 0U)) {
        return DIAGNOSTIC_I2C_INVALID_ARGUMENT;
    }

    status = DiagnosticI2c_Begin(address7Bit, true);
    if (status != DIAGNOSTIC_I2C_OK) {
        DiagnosticI2c_Stop();
        return status;
    }
    return DiagnosticI2c_ReceiveAfterAddress(data, size);
}

/** =================================================================*
 * @brief  DiagnosticI2c_WriteRead処理
 * @param[in] address7Bit 引数
 * @param[in] writeData 引数
 * @param[in] writeSize 引数
 * @param[out] readData 引数
 * @param[in] readSize 引数
 * @return 実行結果または取得値
 * ================================================================= */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_WriteRead(
    uint8_t address7Bit,
    const uint8_t *writeData,
    uint16_t writeSize,
    uint8_t *readData,
    uint16_t readSize) {
    DIAGNOSTIC_I2C_STATUS status;
    uint16_t index;
    uint32_t control;

    if ((address7Bit > 0x7FU) || (writeData == NULL) || (writeSize == 0U) ||
        (readData == NULL) || (readSize == 0U)) {
        return DIAGNOSTIC_I2C_INVALID_ARGUMENT;
    }

    status = DiagnosticI2c_Begin(address7Bit, false);
    for (index = 0U; (index < writeSize) && (status == DIAGNOSTIC_I2C_OK); ++index) {
        if (index == (uint16_t)(writeSize - 1U)) {
            set_bit(I2CF0->I2F0CTL, I2C_CONTROL_CLOCK_HOLD_MASK);
        }
        write_reg32(I2CF0->I2F0DR, writeData[index]);
        status = DiagnosticI2c_WaitTransmitComplete();
    }
    if (status != DIAGNOSTIC_I2C_OK) {
        DiagnosticI2c_Stop();
        return status;
    }

    /* SCL保持を解除し、マスター受信へ切り替える。 */
    control = read_reg32(I2CF0->I2F0CTL);
    control |= I2C_CONTROL_REPEAT_MASK;
    control &= ~(I2C_CONTROL_TRANSMIT_MASK | I2C_CONTROL_CLOCK_HOLD_MASK);
    write_reg32(I2CF0->I2F0CTL, control);
    write_reg32(I2CF0->I2F0DR, ((uint32_t)address7Bit << 1U) | 1UL);

    status = DiagnosticI2c_WaitTransmitComplete();
    if (status != DIAGNOSTIC_I2C_OK) {
        DiagnosticI2c_Stop();
        return status;
    }
    return DiagnosticI2c_ReceiveAfterAddress(readData, readSize);
}
