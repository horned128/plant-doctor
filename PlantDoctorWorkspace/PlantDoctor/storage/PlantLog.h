#ifndef PLANT_LOG_H
#define PLANT_LOG_H

#include <stdbool.h>
#include <stdint.h>

bool PlantLog_Init(void);
bool PlantLog_Append(const uint8_t *data, uint16_t size);
void PlantLog_Process10Ms(void);

#endif /* PLANT_LOG_H */
