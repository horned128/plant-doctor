/*****************************************************************************
 codeoption.c

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
    202.12.20 Ver 1.1.0

******************************************************************************/
/**
 * @file    codeoption.c
 *
 * This file is the code option.
 *
 */
#include "codeoption_config.h"

/*############################################################################*/
/*#                         Code option                                      #*/
/*############################################################################*/

const uint32_t codeop_area[CODEOPTION_AREA_SIZE] __attribute__((section(".codeoption"))) ={
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    ((0x1FFFFFFFUL << 3UL) | (CODEOPTION0_WDTPWMD0 << 2UL) | (0x1UL << 1UL) | (CODEOPTION0_WDTMD)),
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL
};


