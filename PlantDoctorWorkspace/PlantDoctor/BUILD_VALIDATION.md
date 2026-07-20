# Build validation

## Validated environment

- LEXIDE-Ω 2.2.0
- LEXIDE Arm BuildTools `Ver.20260317`
- ARM CMSIS 5.9.0
- ROHM ML63Q25x7_DFP 1.1.0
- Target: ML63Q2557 / Arm Cortex-M0+

## Result

All 38 C translation units from PlantDoctor and its linked CommonFiles were
compiled from scratch with `-mcpu=cortex-m0plus`. The generated assembly was
assembled, linked with the project linker script and LEXIDE runtime libraries,
and converted to Intel HEX.

| Configuration | Optimization | Failures | Warnings | `text` | `data` | `bss` | `dec` |
|---|---:|---:|---:|---:|---:|---:|---:|
| Debug | `-O0`, DWARF 4 | 0 | 0 | 12,407 | 4,100 | 100 | 16,607 |
| Release | `-O2` | 0 | 0 | 9,552 | 4,104 | 104 | 13,760 |

The size report includes the linker-script heap reservation. Application code
does not call `malloc`, `calloc`, `realloc`, `free`, or C++ allocation APIs.

For both configurations, ELF inspection confirms:

- Arm EABI executable for Cortex-M0+ with `Reset_Handler` as the entry path.
- Strong `TM0_IRQHandler` and `TM1_IRQHandler` definitions are linked.
- `.codeoption` is exactly 64 bytes at `0x1003FFC0`.
- `PlantDoctor.elf` and `PlantDoctor.hex` are generated under the selected
  build directory; these generated artifacts are intentionally Git-ignored.

## Hardware-debug correction (2026-07-20)

The first hardware run entered `APP_STATE_ERROR` with
`PLANT_DOCTOR_ERROR_TIMER`, although Timer0 subsequently counted normally.
`TMSTAT` is synchronized to LSCLK and did not report the running state
immediately after `timer0_start()`. `BoardTimer_Init()` now waits for the status
with a bounded timeout instead of treating the first status read as a failure.

Both configurations were rebuilt from all 38 C translation units after this
correction. The table above records the corrected-build sizes. The target must
still be reprogrammed with this build before the LCD and switch path can be
verified on hardware.

## LEXIDE headless note

LEXIDE's installed Arm managed-build plug-in accesses the Eclipse graphical
Workbench while evaluating target options. Consequently, CDT's headless-build
application cannot finish makefile generation in this LEXIDE release. The
compile/link validation above invokes the same installed LEXIDE compiler,
assembler, linker, runtime libraries, device header, linker script, and CMSIS
pack directly. The normal graphical LEXIDE import/build path remains the
intended workflow and should be run once on the development PC before hardware
download.

## Debug-probe connectivity

Windows reports the connected MCU-Link as `MCU-LINK (r0FB) CMSIS-DAP V3.172`.
Using LEXIDE's OpenOCD 0.12.0 with `cmsis-dap.cfg` and the device pack's
`ml63q25x7.cfg` succeeded at 500 kHz SWD: DPIDR `0x0BC11477` was read and a
Cortex-M0+ r0p1 target with four breakpoints and two watchpoints was detected.
This check did not erase or program target flash; download and on-board behavior
still require the explicit hardware procedure in `README.md`.
