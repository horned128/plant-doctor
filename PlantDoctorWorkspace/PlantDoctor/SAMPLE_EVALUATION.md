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
I2C waits are bounded, and byte transfer occurs in the main context instead of
inside the I2CF0 interrupt handler. CommonFiles remains unchanged.

## Removal decision

After Debug and Release validation completed with no PlantDoctor references to
`SampleProject/`, all nine sample workspaces were removed. Their relevant pin
assignments and behavior are now represented by PlantDoctor adapters, while the
reusable vendor implementation remains in CommonFiles.

| Removed workspace | Reason |
|---|---|
| Lcd | Replaced by the project-local bounded LCD driver and UI wrapper |
| PowerControl | POWER_KEEP is wrapped by `PowerControlAdapter` |
| GPIO | LED, push-switch, and 5 V functions are wrapped under `board/` |
| AnalogSensor | Sensor type and pins are not fixed; only a neutral interface is appropriate now |
| MemsAccelerometer | Not required for bring-up; future input boundary already exists |
| Fram | Storage hardware is not selected; `PlantLog` provides the extension boundary |
| Rtc | Wall-clock logging is deferred and has no current application dependency |
| Uart | FTDI/COM operation is explicitly outside the bring-up scope |
| PowerMonitoringAnalogInput | Supply monitoring is a future option and has no current dependency |

No file with an uncertain runtime dependency was removed: the link map was
produced solely from PlantDoctor plus CommonFiles before deletion. The complete
pre-removal source is recoverable from Git commit `4eebc97`, and the untouched
vendor starting point from `59c6f58`.
