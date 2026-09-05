#include "PlantLog.h"

bool PlantLog_Init(void)
{
	/* FRAM allocation and record format are intentionally deferred. */
	return true;
}

bool PlantLog_Append(const uint8_t *data, uint16_t size)
{
	(void)data;
	(void)size;
	return false;
}

void PlantLog_Process10Ms(void)
{
	/* Future asynchronous storage service boundary. */
}
