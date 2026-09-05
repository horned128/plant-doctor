#include "PowerControlAdapter.h"

#include "Output.h"
#include "SystemPowerControl.h"
#include "mcu.h"
#include "rdwr_reg.h"

#define POWER_KEEP_MASK          (1UL << 5U)
#define REGULATOR_5V_MASK        (1UL << 6U)
#define REGULATOR_5V_MODE        (0x02UL << 16U)

bool PowerControlAdapter_Init(void)
{
	SystemPowerControlInit();

	/* The LCD backlight requires the board's 5 V regulator. */
	set_bit(PORT4->P4MOD1, REGULATOR_5V_MODE);
	(void)OutputOnUInt32(&(PORT4->P4DO), REGULATOR_5V_MASK);

	return PowerControlAdapter_IsPowerHeld() && get_bit(PORT4->P4DO, REGULATOR_5V_MASK);
}

bool PowerControlAdapter_IsPowerHeld(void)
{
	return get_bit(PORT4->P4DO, POWER_KEEP_MASK);
}

void PowerControlAdapter_Shutdown(void)
{
	(void)OutputOffUInt32(&(PORT4->P4DO), REGULATOR_5V_MASK);
	SystemPowerControlFin();
}
