/** =================================================================*
 * @file   FramDriver.h
 * @brief  FeRAM (MB85RS2MTA) ハードウェアドライバ
 * ================================================================= */
#ifndef FRAM_DRIVER_H
#define FRAM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define FRAM_SIZE_BYTES                    (262144UL)  /* 2 Mbit = 256 KiB */
#define FRAM_MAX_ADDRESS                   (0x0003FFFFUL)

/**
 * @brief FeRAM および関連 GPIO の初期化
 */
void FramDriver_Init(void);

/**
 * @brief 1 バイト読み出し
 */
void FramDriver_ReadByte(uint32_t address, uint8_t *byte);

/**
 * @brief 2 バイト読み出し (CPU エンディアン)
 */
void FramDriver_ReadHalfWord(uint32_t address, uint16_t *halfWord);

/**
 * @brief 4 バイト読み出し (CPU エンディアン)
 */
void FramDriver_ReadWord(uint32_t address, uint32_t *word);

/**
 * @brief ブロック読み出し
 */
void FramDriver_ReadBlock(uint32_t address, void *dst, uint32_t size);

/**
 * @brief 1 バイト書き込み
 */
void FramDriver_WriteByte(uint32_t address, uint8_t byte);

/**
 * @brief 2 バイト書き込み (CPU エンディアン)
 */
void FramDriver_WriteHalfWord(uint32_t address, uint16_t halfWord);

/**
 * @brief 4 バイト書き込み (CPU エンディアン)
 */
void FramDriver_WriteWord(uint32_t address, uint32_t word);

/**
 * @brief ブロック書き込み
 */
void FramDriver_WriteBlock(uint32_t address, const void *src, uint32_t size);

#endif /* FRAM_DRIVER_H */
