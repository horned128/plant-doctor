#include "PlantAi.h"

#include "PlantFeature.h"

static PLANT_FEATURE_VECTOR s_feature;

bool PlantAi_Init(void)
{
	PlantFeature_Reset(&s_feature);
	return true;
}

void PlantAi_Process10Ms(void)
{
	/* MlTask, Preprocess and AnomalyDetector will be connected here later. */
}

bool PlantAi_IsAnomaly(void)
{
	return false;
}
