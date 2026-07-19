#ifndef PLANT_AI_H
#define PLANT_AI_H

#include <stdbool.h>

bool PlantAi_Init(void);
void PlantAi_Process10Ms(void);
bool PlantAi_IsAnomaly(void);

#endif /* PLANT_AI_H */
