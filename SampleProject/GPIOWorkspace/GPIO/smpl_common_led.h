/*****************************************************************************
 smpl_common_led.h

 Copyright (C) 2024 ROHM Co., Ltd.
 All rights reserved.

 This software is provided "as is" and any expressed or implied
 warranties, including, but not limited to, the implied warranties of
 merchantability and fitness for a particular purpose are disclaimed.
 ROHM shall not be liable for any direct, indirect, consequential or
 incidental damages arising from using or modifying this software.
 You (customer) can modify and use this software in whole or part on
 your own responsibility, only for the purpose of developing the software
 for use with microcontroller manufactured by ROHM.

 History
    2024.07.31 Ver 1.0.0

******************************************************************************/
/*****************************************************************************
 * File: smpl_common_led.h
 * Title: LEDを制御する。 
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file smpl_common_led.h
 * @brief LEDを制御する。 
 */
#ifndef SMPL_COMMON_LED_H__
#define SMPL_COMMON_LED_H__

#include "mcu.h"
#include "rdwr_reg.h"

#define         LED1_PORT                   ( PORT5->P5DO )                         /**< LEDポート1 */
#define         LED2_PORT                   ( PORT5->P5DO )                         /**< LEDポート2 */
#define         LED3_PORT                   ( PORT5->P5DO )                         /**< LEDポート3 */
#define         LED_ACTIVE                  ( 1U )                                  /**< ON */
#define         LED_INACTIVE                ( 0U )                                  /**< OFF */


/**
 * @brief LED1をONにする。
 */
#define         smpl_onLED1()               ( set_bit( LED1_PORT, (LED_ACTIVE << 4U) ) )

/**
 * @brief LED1をOFFにする。
 */
#define         smpl_offLED1()              ( clear_bit( LED1_PORT, (LED_ACTIVE << 4U) ) )

/**
 * @brief LED1を点滅させる。
 */
#define         smpl_blinkLED1()            do {                                                        \
                                                if( get_bit( LED1_PORT, (1U << 4U) ) != 1 ) {           \
                                                    smpl_onLED1();                                      \
                                                } else {                                                \
                                                    smpl_offLED1();                                     \
                                                }                                                       \
                                            } while(0)

/**
 * @brief LED2をONにする。
 */
#define         smpl_onLED2()               ( set_bit( LED2_PORT, (LED_ACTIVE << 5U) ) )

/**
 * @brief LED2をOFFにする。
 */
#define         smpl_offLED2()              ( clear_bit( LED2_PORT, (LED_ACTIVE << 5U) ) )

/**
 * @brief LED2を点滅させる。
 */
#define         smpl_blinkLED2()            do {                                                        \
                                                if( get_bit( LED2_PORT, (1U << 5U) ) != 1 ) {           \
                                                    smpl_onLED2();                                      \
                                                } else {                                                \
                                                    smpl_offLED2();                                     \
                                                }                                                       \
                                            } while(0)

/**
 * @brief LED3をONにする。
 */
#define         smpl_onLED3()               ( set_bit( LED3_PORT, (LED_ACTIVE << 6U) ) )

/**
 * @brief LED3をOFFにする。
 */
#define         smpl_offLED3()              ( clear_bit( LED3_PORT, (LED_ACTIVE << 6U) ) )

/**
 * @brief LED3を点滅させる。
 */
#define         smpl_blinkLED3()            do {                                                        \
                                                if( get_bit( LED3_PORT, (1U << 6U) ) != 1 ) {           \
                                                    smpl_onLED3();                                      \
                                                } else {                                                \
                                                    smpl_offLED3();                                     \
                                                }                                                       \
                                            } while(0)



/**
 * @brief LED1を初期化する。
 *
 * @param defVal 状態
 */
void smpl_initLED1( uint8_t defVal );

/**
 * @brief LED2を初期化する。
 *
 * @param defVal 状態
 */
void smpl_initLED2( uint8_t defVal );

/**
 * @brief LED3を初期化する。
 *
 * @param defVal 状態
 */
void smpl_initLED3( uint8_t defVal );

#endif //SMPL_COMMON_LED_H__

