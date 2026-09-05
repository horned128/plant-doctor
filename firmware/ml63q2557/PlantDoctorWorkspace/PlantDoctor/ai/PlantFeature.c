#include "PlantFeature.h"

void PlantFeature_Reset(PLANT_FEATURE_VECTOR *feature)
{
	if (feature != 0)
	{
		feature->soilMoisture = 0;
		feature->soilMoistureMovingAverage = 0;
		feature->soilMoistureRate = 0;
		feature->leafAirTemperatureDelta = 0;
		feature->elapsedSinceWateringSeconds = 0U;
	}
}
