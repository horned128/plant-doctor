# Plant Doctor firmware

Bare-metal bring-up firmware for the DATA TECNO DT-EBML63Q2557 board
(ROHM ML63Q2557, Arm Cortex-M0+). The project targets LEXIDE-Ω and MCU-Link
over CMSIS-DAP/SWD.

```text
CommonFiles/                     Vendor IODriver modules (kept unchanged)
PlantDoctorWorkspace/
  PlantDoctor/                   LEXIDE-Ω project
CODEX.md                         Product and implementation requirements
```

Start with [PlantDoctorWorkspace/PlantDoctor/README.md](PlantDoctorWorkspace/PlantDoctor/README.md)
for import, build, programming, and hardware-check instructions. Build evidence
is recorded in
[BUILD_VALIDATION.md](PlantDoctorWorkspace/PlantDoctor/BUILD_VALIDATION.md).

The original vendor samples were removed only after PlantDoctor built without
their source trees. They remain recoverable from Git commit `59c6f58`; the
comparison and removal rationale are recorded in
[SAMPLE_EVALUATION.md](PlantDoctorWorkspace/PlantDoctor/SAMPLE_EVALUATION.md).
