/** =================================================================*
 * @file   FramDriver.c
 * @brief  FeRAM (MB85RS2MTA) ハードウェアドライバ
 * ================================================================= */
#include "FramDriver.h"

#if defined(__arm__)
#include "SoftSpi.h"
#include "mcu.h"
#include "rdwr_reg.h"

#define SET_PORT_OUTPUT_WP()               (set_bit(PORT3->P3MOD0, (1 << 1U)))
#define WP_OFF()                           (set_bit(PORT3->P3DO, (1 << 0U)))
#define WP_ON()                            (clear_bit(PORT3->P3DO, (1 << 0U)))

#define CMD_WREN                           (0x06U)
#define CMD_WRDI                           (0x04U)
#define CMD_WRSR                           (0x01U)
#define CMD_READ                           (0x03U)
#define CMD_WRITE                          (0x02U)

static void sendCommandWREN(void)
{
    const uint8_t cmd = CMD_WREN;
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    SoftSpiWrite(&cmd, 1);
    SoftSpiDeviceDisable();
}

static void sendCommandWRDI(void)
{
    const uint8_t cmd = CMD_WRDI;
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    SoftSpiWrite(&cmd, 1);
    SoftSpiDeviceDisable();
}

static void sendCommandWrsrUnprotect(void)
{
    const uint8_t cmd[2] = {CMD_WRSR, 0x00U};
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    SoftSpiWrite(cmd, 2);
    SoftSpiDeviceDisable();
}

static void sendCommandRead(uint32_t address)
{
    uint8_t sendData[4];
    sendData[0] = CMD_READ;
    sendData[1] = (uint8_t)((address >> 16U) & 0xFFU);
    sendData[2] = (uint8_t)((address >> 8U) & 0xFFU);
    sendData[3] = (uint8_t)(address & 0xFFU);
    SoftSpiWrite(sendData, 4);
}

static void sendCommandWrite(uint32_t address)
{
    uint8_t sendData[4];
    sendData[0] = CMD_WRITE;
    sendData[1] = (uint8_t)((address >> 16U) & 0xFFU);
    sendData[2] = (uint8_t)((address >> 8U) & 0xFFU);
    sendData[3] = (uint8_t)(address & 0xFFU);
    SoftSpiWrite(sendData, 4);
}

void FramDriver_Init(void)
{
    SET_PORT_OUTPUT_WP();
    WP_OFF();
    sendCommandWREN();
    sendCommandWrsrUnprotect();
    sendCommandWRDI();
}

void FramDriver_ReadByte(uint32_t address, uint8_t *byte)
{
    if (byte == (void *)0) {
        return;
    }
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandRead(address);
    SoftSpiRead(byte, 1, 0x00U);
    SoftSpiDeviceDisable();
}

void FramDriver_ReadHalfWord(uint32_t address, uint16_t *halfWord)
{
    if (halfWord == (void *)0) {
        return;
    }
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandRead(address);
    SoftSpiRead(halfWord, 2, 0x00U);
    SoftSpiDeviceDisable();
}

void FramDriver_ReadWord(uint32_t address, uint32_t *word)
{
    if (word == (void *)0) {
        return;
    }
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandRead(address);
    SoftSpiRead(word, 4, 0x00U);
    SoftSpiDeviceDisable();
}

void FramDriver_ReadBlock(uint32_t address, void *dst, uint32_t size)
{
    if (dst == (void *)0 || size == 0U) {
        return;
    }
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandRead(address);
    SoftSpiRead(dst, (int)size, 0x00U);
    SoftSpiDeviceDisable();
}

void FramDriver_WriteByte(uint32_t address, uint8_t byte)
{
    sendCommandWREN();
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandWrite(address);
    SoftSpiWrite(&byte, 1);
    SoftSpiDeviceDisable();
    sendCommandWRDI();
}

void FramDriver_WriteHalfWord(uint32_t address, uint16_t halfWord)
{
    sendCommandWREN();
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandWrite(address);
    SoftSpiWrite(&halfWord, 2);
    SoftSpiDeviceDisable();
    sendCommandWRDI();
}

void FramDriver_WriteWord(uint32_t address, uint32_t word)
{
    sendCommandWREN();
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandWrite(address);
    SoftSpiWrite(&word, 4);
    SoftSpiDeviceDisable();
    sendCommandWRDI();
}

void FramDriver_WriteBlock(uint32_t address, const void *src, uint32_t size)
{
    if (src == (void *)0 || size == 0U) {
        return;
    }
    sendCommandWREN();
    SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
    sendCommandWrite(address);
    SoftSpiWrite(src, (int)size);
    SoftSpiDeviceDisable();
    sendCommandWRDI();
}

#else
/* ホストシミュレーション環境用メモリバッキング */
#include <string.h>

static uint8_t s_simulatedFram[FRAM_SIZE_BYTES];

void FramDriver_Init(void)
{
    /* シミュレーション用初期化 */
}

void FramDriver_ReadByte(uint32_t address, uint8_t *byte)
{
    if (byte != (void *)0 && address < FRAM_SIZE_BYTES) {
        *byte = s_simulatedFram[address];
    }
}

void FramDriver_ReadHalfWord(uint32_t address, uint16_t *halfWord)
{
    if (halfWord != (void *)0 && (address + 1U) < FRAM_SIZE_BYTES) {
        (void)memcpy(halfWord, &s_simulatedFram[address], 2);
    }
}

void FramDriver_ReadWord(uint32_t address, uint32_t *word)
{
    if (word != (void *)0 && (address + 3U) < FRAM_SIZE_BYTES) {
        (void)memcpy(word, &s_simulatedFram[address], 4);
    }
}

void FramDriver_ReadBlock(uint32_t address, void *dst, uint32_t size)
{
    if (dst != (void *)0 && (address + size) <= FRAM_SIZE_BYTES) {
        (void)memcpy(dst, &s_simulatedFram[address], size);
    }
}

void FramDriver_WriteByte(uint32_t address, uint8_t byte)
{
    if (address < FRAM_SIZE_BYTES) {
        s_simulatedFram[address] = byte;
    }
}

void FramDriver_WriteHalfWord(uint32_t address, uint16_t halfWord)
{
    if ((address + 1U) < FRAM_SIZE_BYTES) {
        (void)memcpy(&s_simulatedFram[address], &halfWord, 2);
    }
}

void FramDriver_WriteWord(uint32_t address, uint32_t word)
{
    if ((address + 3U) < FRAM_SIZE_BYTES) {
        (void)memcpy(&s_simulatedFram[address], &word, 4);
    }
}

void FramDriver_WriteBlock(uint32_t address, const void *src, uint32_t size)
{
    if (src != (void *)0 && (address + size) <= FRAM_SIZE_BYTES) {
        (void)memcpy(&s_simulatedFram[address], src, size);
    }
}

#endif
