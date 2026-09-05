/** =================================================================*
 * @file   PlantLog.h
 * @brief  植物ログ保存API
 * ================================================================= */
#ifndef PLANT_LOG_H
#define PLANT_LOG_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

bool PlantLog_Init(void);                                   /* PlantLog_InitのAPI */
bool PlantLog_Append(const uint8_t *data, uint16_t size);   /* PlantLog_AppendのAPI */
void PlantLog_Process10Ms(void);                            /* PlantLog_Process10MsのAPI */

#endif /* PLANT_LOG_H */
