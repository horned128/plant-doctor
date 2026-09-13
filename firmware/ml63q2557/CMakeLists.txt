cmake_minimum_required(VERSION 3.20)

# Keep toolchain selection before project() so command-line builds also work
# without a preset.
if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-toolchain.cmake"
      CACHE FILEPATH "ARM GNU toolchain file")
endif()

project(PlantDoctor LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PLANT_DOCTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PlantDoctorWorkspace/PlantDoctor")
set(COMMON_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CommonFiles")

# Prefer a repository-local CMSIS checkout, while also supporting the CMSIS
# pack already installed by LEXIDE on Windows.
set(CMSIS_CORE_INCLUDE "" CACHE PATH "Directory containing core_cm0plus.h")
if(NOT CMSIS_CORE_INCLUDE)
  set(_cmsis_candidates
      "${CMAKE_CURRENT_SOURCE_DIR}/../../external/CMSIS/CMSIS/Core/Include")

  if(DEFINED ENV{CMSIS_ROOT})
    list(APPEND _cmsis_candidates "$ENV{CMSIS_ROOT}/CMSIS/Core/Include")
  endif()

  if(DEFINED ENV{LOCALAPPDATA})
    file(GLOB _cmsis_pack_candidates
         "$ENV{LOCALAPPDATA}/Arm/Packs/ARM/CMSIS/*/CMSIS/Core/Include")
    list(SORT _cmsis_pack_candidates COMPARE NATURAL ORDER DESCENDING)
    list(APPEND _cmsis_candidates ${_cmsis_pack_candidates})
  endif()

  foreach(_candidate IN LISTS _cmsis_candidates)
    if(EXISTS "${_candidate}/core_cm0plus.h")
      set(CMSIS_CORE_INCLUDE "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(NOT EXISTS "${CMSIS_CORE_INCLUDE}/core_cm0plus.h")
  message(FATAL_ERROR
    "CMSIS Core headers were not found. Install the ARM CMSIS pack, set "
    "CMSIS_CORE_INCLUDE, or clone CMSIS_6 under external/CMSIS.")
endif()
message(STATUS "CMSIS Core: ${CMSIS_CORE_INCLUDE}")

set(PLANT_DOCTOR_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/gcc_syscalls.c
  ${PLANT_DOCTOR_DIR}/src/main.c
  ${PLANT_DOCTOR_DIR}/NmiHandler.c
  ${PLANT_DOCTOR_DIR}/codeoption.c
  ${PLANT_DOCTOR_DIR}/startup_ML63Q25x7.c
  ${PLANT_DOCTOR_DIR}/system_ML63Q25x7.c
  ${PLANT_DOCTOR_DIR}/actuator/PumpControl.c
  ${PLANT_DOCTOR_DIR}/ai/PlantAi.c
  ${PLANT_DOCTOR_DIR}/ai/PlantDiagnosis.c
  ${PLANT_DOCTOR_DIR}/ai/PlantFeature.c
  ${PLANT_DOCTOR_DIR}/ai/PlantStress.c
  ${PLANT_DOCTOR_DIR}/ai/WateringResponse.c
  ${PLANT_DOCTOR_DIR}/app/App.c
  ${PLANT_DOCTOR_DIR}/app/AppStateMachine.c
  ${PLANT_DOCTOR_DIR}/app/DemoMode.c
  ${PLANT_DOCTOR_DIR}/app/TimeKeeper.c
  ${PLANT_DOCTOR_DIR}/app/WateringPolicy.c
  ${PLANT_DOCTOR_DIR}/board/Board.c
  ${PLANT_DOCTOR_DIR}/board/BoardTimer.c
  ${PLANT_DOCTOR_DIR}/board/LedControl.c
  ${PLANT_DOCTOR_DIR}/board/PowerControlAdapter.c
  ${PLANT_DOCTOR_DIR}/board/RtcRx4111.c
  ${PLANT_DOCTOR_DIR}/board/SwitchControl.c
  ${PLANT_DOCTOR_DIR}/console/Console.c
  ${PLANT_DOCTOR_DIR}/console/ConsoleUart.c
  ${PLANT_DOCTOR_DIR}/drivers/Lcd.c
  ${PLANT_DOCTOR_DIR}/drivers/LcdI2cf0.c
  ${PLANT_DOCTOR_DIR}/sensors/EnvironmentSensor.c
  ${PLANT_DOCTOR_DIR}/sensors/I2cBus.c
  ${PLANT_DOCTOR_DIR}/sensors/EbmlI2cTelemetry.c
  ${PLANT_DOCTOR_DIR}/sensors/LeafTemperatureSensor.c
  ${PLANT_DOCTOR_DIR}/sensors/SensorManager.c
  ${PLANT_DOCTOR_DIR}/sensors/SensorPlausibility.c
  ${PLANT_DOCTOR_DIR}/sensors/SoilCalibration.c
  ${PLANT_DOCTOR_DIR}/sensors/SoilMoistureSensor.c
  ${PLANT_DOCTOR_DIR}/sensors/TankLevelSensor.c
  ${PLANT_DOCTOR_DIR}/storage/FramDriver.c
  ${PLANT_DOCTOR_DIR}/storage/PlantLog.c
  ${PLANT_DOCTOR_DIR}/storage/PlantLogQueue.c
  ${PLANT_DOCTOR_DIR}/storage/PlantLogRecord.c
  ${PLANT_DOCTOR_DIR}/storage/PlantLogRing.c
  ${PLANT_DOCTOR_DIR}/ui/LcdUi.c
)

set(VENDOR_SOURCES
  ${COMMON_FILES_DIR}/Driver/irq.c
  ${COMMON_FILES_DIR}/Driver/saAdc0.c
  ${COMMON_FILES_DIR}/Driver/saAdc1.c
  ${COMMON_FILES_DIR}/Driver/smpl_common.c
  ${COMMON_FILES_DIR}/Driver/ssiof0.c
  ${COMMON_FILES_DIR}/Driver/timer0_1.c
  ${COMMON_FILES_DIR}/Driver/uartf1.c
  ${COMMON_FILES_DIR}/Driver/wdt.c
  ${COMMON_FILES_DIR}/Power/Input.c
  ${COMMON_FILES_DIR}/Power/Output.c
  ${COMMON_FILES_DIR}/Power/PowerKeep.c
  ${COMMON_FILES_DIR}/Power/PowerMonitoringSw.c
  ${COMMON_FILES_DIR}/Power/SystemPowerControl.c
  ${COMMON_FILES_DIR}/SoftSpi/SoftSpi.c
  ${COMMON_FILES_DIR}/Timer/TimeControl.c
)

# Keep vendor sources unchanged while avoiding warnings about their established
# declaration style. Warnings remain enabled for Plant Doctor-owned sources.
set_source_files_properties(${VENDOR_SOURCES} PROPERTIES
  COMPILE_OPTIONS "-Wno-old-style-declaration")

set(CPU_OPTIONS
  -mcpu=cortex-m0plus
  -mthumb
  -fno-signed-char
  -fno-short-enums
)

# The LEXIDE linker file contains the verified ML63Q2557 memory map and code
# option placement. GNU ld should treat the reserved heap/stack as non-loadable
# sections, so generate a GCC-specific copy in the build directory.
set(LEXIDE_LINKER_SCRIPT "${PLANT_DOCTOR_DIR}/ML63Q25x7_lccarm.ld")
set(GCC_LINKER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/ML63Q25x7_gcc.ld")
file(READ "${LEXIDE_LINKER_SCRIPT}" _linker_script)
string(REPLACE ".heap :" ".heap (COPY) :" _linker_script "${_linker_script}")
string(REGEX REPLACE "(\\.stack \\([^\\r\\n]+\\)) :" "\\1 (COPY) :"
       _linker_script "${_linker_script}")
file(WRITE "${GCC_LINKER_SCRIPT}" "${_linker_script}")

function(configure_ml63q2557_firmware target)
  set_target_properties(${target} PROPERTIES SUFFIX ".elf")

  target_include_directories(${target} PRIVATE
    ${PLANT_DOCTOR_DIR}
    ${PLANT_DOCTOR_DIR}/actuator
    ${PLANT_DOCTOR_DIR}/ai
    ${PLANT_DOCTOR_DIR}/ai/inc
    ${PLANT_DOCTOR_DIR}/app
    ${PLANT_DOCTOR_DIR}/board
    ${PLANT_DOCTOR_DIR}/config
    ${PLANT_DOCTOR_DIR}/console
    ${PLANT_DOCTOR_DIR}/drivers
    ${PLANT_DOCTOR_DIR}/inc
    ${PLANT_DOCTOR_DIR}/sensors
    ${PLANT_DOCTOR_DIR}/storage
    ${PLANT_DOCTOR_DIR}/ui
    ${COMMON_FILES_DIR}/Driver
    ${COMMON_FILES_DIR}/Power
    ${COMMON_FILES_DIR}/SoftSpi
    ${COMMON_FILES_DIR}/Timer
    ${CMSIS_CORE_INCLUDE}
  )

  target_compile_definitions(${target} PRIVATE
    ML63Q25x7
    ML63Q2557
    ODL_MAX_INST_NUM=1
    ODL_MAX_INPUTS=512
    ODL_MAX_UNITS=64
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
  )

  target_compile_options(${target} PRIVATE
    ${CPU_OPTIONS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-gdwarf-4>
    $<$<CONFIG:Release>:-Os>
  )

  target_link_options(${target} PRIVATE
    ${CPU_OPTIONS}
    --specs=nano.specs
    "-T${GCC_LINKER_SCRIPT}"
    -Wl,--gc-sections
    -Wl,--no-enum-size-warning
    -Wl,-z,noexecstack
    -Wl,--print-memory-usage
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map"
  )
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${LEXIDE_LINKER_SCRIPT}")

  target_link_libraries(${target} PRIVATE
    "${PLANT_DOCTOR_DIR}/ai/lib/SolistAi_Library_1_512_64.a"
  )

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${target}> "${CMAKE_CURRENT_BINARY_DIR}/${target}.hex"
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${target}> "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
    COMMENT "Generating ${target}.hex and ${target}.bin"
  )
endfunction()

add_executable(PlantDoctor ${PLANT_DOCTOR_SOURCES} ${VENDOR_SOURCES})
configure_ml63q2557_firmware(PlantDoctor)
