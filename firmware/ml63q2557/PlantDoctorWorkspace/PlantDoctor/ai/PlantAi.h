/** =================================================================*
 * @file   PlantAi.h
 * @brief  植物状態AI API
 * ================================================================= */
#ifndef PLANT_AI_H
#define PLANT_AI_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */

bool PlantAi_Init(void);                                    /* PlantAi_InitのAPI */
void PlantAi_Process10Ms(void);                             /* PlantAi_Process10MsのAPI */
bool PlantAi_IsAnomaly(void);                               /* PlantAi_IsAnomalyのAPI */

#endif /* PLANT_AI_H */
