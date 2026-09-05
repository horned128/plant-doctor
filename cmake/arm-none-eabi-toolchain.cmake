set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_arm_toolchain_hints)
if(DEFINED ENV{ARM_GCC_ROOT})
  list(APPEND _arm_toolchain_hints "$ENV{ARM_GCC_ROOT}/bin")
endif()
if(DEFINED ENV{USERPROFILE})
  list(APPEND _arm_toolchain_hints
       "$ENV{USERPROFILE}/.platformio/packages/toolchain-gccarmnoneeabi/bin")
endif()

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc
             HINTS ${_arm_toolchain_hints} REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc
             HINTS ${_arm_toolchain_hints} REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy
             HINTS ${_arm_toolchain_hints} REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size
             HINTS ${_arm_toolchain_hints} REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
