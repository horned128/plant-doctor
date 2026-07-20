# Hardware diagnostics

## 2026-07-20 first-board result

The original firmware was built, programmed, and started successfully through
LEXIDE and MCU-Link. LEDs changed, but the LCD was blank and switch presses had
no visible effect.

A read-only inspection through the active OpenOCD session found:

- `s_state == APP_STATE_ERROR` (`3`).
- `s_error == PLANT_DOCTOR_ERROR_TIMER` (`2`).
- Timer0 was counting and `TMSTAT == 1` after startup.
- LCD-related P7 and I2CF0 registers were still zero, showing that LCD
  initialization had not been reached.
- The P3/P5 switch inputs were configured and read high, but switch feedback
  is intentionally inactive while the application is in `APP_STATE_ERROR`.

The apparent LED activity was the 250 ms alternating error pattern. It was not
the normal one-second LED1 heartbeat.

## Root cause and correction

`BoardTimer_Init()` previously read `timer0_getStatus()` immediately after
`timer0_start()`. The timer status is synchronized to LSCLK, so its first read
can remain zero even though the timer has started correctly. This transient
value was reported as `PLANT_DOCTOR_ERROR_TIMER`, preventing the state machine
from reaching LCD initialization and switch monitoring.

`BoardTimer_Init()` now waits for the running status with the bounded
`PLANT_DOCTOR_TIMER_START_TIMEOUT_LOOPS` timeout. The interrupt handler remains
limited to updating tick state; no LCD, switch, or other peripheral work was
moved into interrupt context.

## Retest

1. Terminate the active LEXIDE debug session using the red **Terminate** button.
2. Run **Project > Clean**, then **Project > Build Project**.
3. Confirm that `Debug/PlantDoctor.elf` has a new timestamp.
4. Start the PlantDoctor debug configuration to download and verify flash.
5. Resume execution and reset or power-cycle the target if necessary.
6. Confirm `PLANT DOCTOR` / `BOARD TEST`, the one-second LED1 heartbeat, and
   SW1 through SW4 feedback.

If the corrected build still fails, halt the target and inspect
`AppStateMachine.c` globals `s_state` and `s_error`. Relevant error values are:

| Value | Meaning |
|---:|---|
| 0 | `PLANT_DOCTOR_ERROR_NONE` |
| 1 | `PLANT_DOCTOR_ERROR_POWER` |
| 2 | `PLANT_DOCTOR_ERROR_TIMER` |
| 3 | `PLANT_DOCTOR_ERROR_SWITCH` |
| 4 | `PLANT_DOCTOR_ERROR_LCD_INIT` |
| 5 | `PLANT_DOCTOR_ERROR_LCD_IO` |

For LCD errors, inspect P4.6 (5 V enable), P7.5 (backlight), P7.2 (LCD reset),
P7.3/P7.4 (SCLF0/SDAF0), and I2CF0 ACK status. Record `s_state`, `s_error`, and
the observed LED pattern before changing the peripheral drivers.
