/*****************************************************************************
 codeoption.h

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
/**
 * @file    codeoption.h
 * 
 * This file defines code option.
 * 
 */

#ifndef	CODEOPTION_H__
#define	CODEOPTION_H__

#include "mcu.h"

/*############################################################################*/
/*#                                  Macro                                   #*/
/*############################################################################*/
#define	CODEOPTION_AREA_SIZE					( 16U )						/**< code option area size           */

/* CODE OPTION 0 */
#define	CODEOPTION0_WDTMD_DISABLED 				( 0UL )						/**< WDTMD : disabled                */
#define	CODEOPTION0_WDTMD_ENABLED 				( 1UL )						/**< WDTMD : enabled                 */

#define	CODEOPTION0_WDTPWMD0_DISABLED			( 0UL )						/**< WDTPWMD0 : disabled             */
#define	CODEOPTION0_WDTPWMD0_ENABLED			( 1UL )						/**< WDTPWMD0 : enabled              */


extern const uint32_t codeop_area[CODEOPTION_AREA_SIZE];

#endif	/* CODEOPTION_H__ */
