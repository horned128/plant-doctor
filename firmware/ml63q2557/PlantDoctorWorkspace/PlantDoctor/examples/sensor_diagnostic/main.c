#include "SensorDiagnostic.h"

int main(void)
{
	SensorDiagnostic_Init();

	while (1)
	{
		SensorDiagnostic_RunOnce();
	}
}
