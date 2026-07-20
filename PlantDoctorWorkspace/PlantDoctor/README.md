# Plant Doctor bring-up firmware

This project is the first hardware bring-up firmware for DATA TECNO's
DT-EBML63Q2557 board. It uses a bare-metal superloop and an explicit state
machine. Dynamic allocation is not used.

## Bring-up behavior

- Enables `POWER_KEEP` and the board 5 V regulator.
- Runs Timer0 as a 10 ms auto-reload system tick.
- Blinks LED1 once per second in `MONITOR`.
- Initializes the LCD and displays:

  ```text
  PLANT DOCTOR
  BOARD TEST
  ```

- Debounces the four push switches every 10 ms. Pressing a switch displays
  `SW1 PRESSED` through `SW4 PRESSED`; LED2/LED3 also indicate switch groups.
- Enters `ERROR` on a detected bring-up fault. The LCD shows an error label
  when available, while LED1/LED2/LED3 show an alternating 250 ms pattern.

Timer0 and Timer1 interrupt handlers only update a counter or completion flag.
LCD I2C transfers are performed synchronously in the main context with bounded
timeouts. Sensor, pump, AI, and storage modules are non-operating extension
points in this revision.

## Architecture

```text
src/main.c
  -> app/App
       -> app/AppStateMachine
       -> board/Board
       -> sensors/SensorManager       (stub boundary)
       -> ai/PlantAi                   (stub boundary)
       -> actuator/PumpControl         (disabled stub)
       -> storage/PlantLog             (stub boundary)

board/Board
  -> CommonFiles/Driver               clock, watchdog, Timer0
  -> CommonFiles/Power                POWER_KEEP, generic input/output

ui/LcdUi
  -> drivers/Lcd
       -> drivers/LcdI2cf0
       -> CommonFiles/Timer            command delays on Timer1
```

Hardware registers are confined to `board/` and `drivers/`. The application
layer depends on their interfaces only.

## LEXIDE-Ω import and build

1. In LEXIDE's CMSIS Pack manager, install ARM CMSIS 5.9.0 and ROHM
   ML63Q25x7_DFP 1.1.0 (or a compatible newer pack).
2. Select **File > Import > General > Existing Projects into Workspace**.
3. Select `PlantDoctorWorkspace/PlantDoctor` as the root directory.
4. Confirm that the project `PlantDoctor` is detected. Leave **Copy projects
   into workspace** cleared so the relative CommonFiles link retains the
   repository layout, then finish the import.
5. Select `Debug` or `Release` under **Build Configurations > Set Active**.
6. If a previous debug session is active, click the red **Terminate** button.
   Suspending the target is not sufficient because OpenOCD/GDB can keep
   `PlantDoctor.elf` open.
7. Run **Project > Clean**, followed by **Project > Build Project**.

Both configurations define `ML63Q25x7` and `ML63Q2557`, use the
`ML63Q25x7_lccarm.ld` linker script, and refer to CommonFiles with relative
paths. No user-specific absolute path is stored in the project.

The project-local linker script retains the 64-byte vendor `.codeoption`
section at `0x1003FFC0`, outside the normal `0x10000000..0x1003FFBF` program
flash region. This prevents link-time garbage collection from removing the
watchdog option words.

## MCU-Link programming checklist

1. Power the DT-EBML63Q2557 through USB Type-C.
2. Connect MCU-Link to the board's SWD signals (SWDIO, SWCLK, GND, and target
   reference voltage). Do not use the MCU-Link target-power output when the
   board is already USB-powered.
3. Connect MCU-Link to the PC through USB.
4. Open **Run > Debug Configurations...**, create a **LAPIS GDB Debugging
   (Arm)** configuration, and select project `PlantDoctor` plus
   `Debug/PlantDoctor.elf` on the Main tab.
5. On the Debugger tab, select `CMSIS-DAP` as the ICE. LEXIDE then uses its
   `cmsis-dap.cfg`; the selected ML63Q25x7 device pack supplies
   `Cfg/ml63q25x7.cfg`, which selects SWD and the 256 KiB flash layout.
6. On the Startup tab, enable loading the project executable and **Verify
   Flash Memory**, then start **Debug**. Resume from `main` after the reset,
   halt, download, and verification sequence finishes.

Exact probe names can vary with the installed LEXIDE/OpenOCD release. Verify
the target-voltage indication before attempting a connection.

## Hardware confirmation

1. Reset or power-cycle the board.
2. Confirm the LCD title and `BOARD TEST` line.
3. Confirm LED1 toggles at a one-second interval. Alternating
   LED1/LED2/LED3 at 250 ms is the error indication, not the normal heartbeat.
4. Press SW1 through SW4 individually and confirm the matching LCD text.
5. Release each switch and confirm that `BOARD TEST` returns.
6. Halt in the debugger and inspect `s_state` in `AppStateMachine.c`; normal
   operation is `APP_STATE_MONITOR` with `PLANT_DOCTOR_ERROR_NONE`.

If the LCD is blank, first check that P4.6 enables the 5 V regulator and P7.5
enables the backlight. Then check P7.2 reset, P7.3 SCLF0, P7.4 SDAF0, I2C ACK
from address `0x7C`, and the state-machine error code. A visible error LED
pattern with no LCD usually means LCD initialization, I2C wiring, or the 5 V
rail failed.

See `HARDWARE_DIAGNOSTICS.md` for the first-board diagnosis and debugger values
to inspect if the corrected build still enters the error state.
