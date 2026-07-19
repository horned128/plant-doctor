/*****************************************************************************
 codeoption_config.h

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
 * @file    codeoption_config.h
 * 
 * This file defines the configuration of code options.
 * 
 */

#ifndef	CODEOPTION_CONFIG_H__
#define	CODEOPTION_CONFIG_H__

#include "codeoption.h"

/*############################################################################*/
/*#                        Code option configuration                         #*/
/*############################################################################*/
/* code option 0 */
#define	CODEOPTION0_WDTMD					( CODEOPTION0_WDTMD_ENABLED )		/**< WDTMD    : CODEOPTION0_WDTMD_DISABLED or CODEOPTION0_WDTMD_ENABLED       */
#define	CODEOPTION0_WDTPWMD0				( CODEOPTION0_WDTPWMD0_ENABLED )	/**< WDTPWMD0 : CODEOPTION0_WDTPWMD0_DISABLED or CODEOPTION0_WDTPWMD0_ENABLED */

#endif	/* CODEOPTION_CONFIG_H__ */
