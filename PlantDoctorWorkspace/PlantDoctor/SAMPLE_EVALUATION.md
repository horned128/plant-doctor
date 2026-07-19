# Vendor sample evaluation

All nine samples use identical ML63Q25x7 device, startup, system, code-option,
and linker files. Each Eclipse project links the repository-wide CommonFiles
directory; its local source supplies the peripheral-specific behavior.

| Sample | Main purpose | Logical CommonFiles dependency | Plant Doctor use |
|---|---|---|---|
| Lcd | I2CF0 character LCD | Driver, Power, Timer | Base project and LCD pin/command reference |
| PowerControl | POWER_KEEP and power-button shutdown | Driver, Power, Timer | Self-hold sequence reference |
| GPIO | LEDs, four push switches, DIP switches, relay/regulators | Driver, Power, Timer | LED, switch, and 5 V rail reference |
| AnalogSensor | ADC0 buffered analog sampling | Driver, Power | Future soil sensor reference only |
| MemsAccelerometer | KX134 over SSIOF0 | Driver, Power, Timer | Future optional vibration input only |
| Fram | FRAM over software SPI | Driver, Power, SoftSpi | Future log storage reference only |
| Rtc | RX4111 over software SPI | Driver, Power, SoftSpi | Future wall-clock reference only |
| Uart | UARTF1 interrupt echo | Driver, Power | Not used in this bring-up |
| PowerMonitoringAnalogInput | ADC1 supply measurement | Driver, Power, Timer | Future supply monitoring reference only |

Lcd was selected as the base because LCD/I2CF0 is the most configuration-
sensitive required peripheral and already includes the correct power, clock,
Timer1, startup, and linker context. GPIO and PowerControl behaviors are small
enough to isolate behind Plant Doctor board adapters. This avoids merging
multiple Eclipse projects while retaining their proven pin assignments.

The project-local LCD driver deliberately differs from the sample in two ways:
all waits are bounded, and byte transfer occurs in the main context instead of
inside the I2CF0 interrupt handler. CommonFiles remains unchanged.
