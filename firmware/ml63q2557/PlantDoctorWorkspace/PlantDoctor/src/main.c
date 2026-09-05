#include "App.h"

int main(void)
{
	App_Init();

	while (1)
	{
		App_RunOnce();
	}
}
