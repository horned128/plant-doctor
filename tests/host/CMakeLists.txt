cmake_minimum_required(VERSION 3.16)
project(PlantDoctorHostTests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX- /utf-8 /wd4127)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

enable_testing()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/sensors
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/board
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/actuator
)

# Smoke test
add_executable(test_smoke
    test_smoke.c
)
add_test(NAME test_smoke COMMAND test_smoke)

# Sample sequence test (P1-1)
add_executable(test_sample_sequence
    test_sample_sequence.c
)
add_test(NAME test_sample_sequence COMMAND test_sample_sequence)

# Soil calibration test (P1-2)
add_executable(test_soil_calibration
    test_soil_calibration.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/sensors/SoilCalibration.c
)
add_test(NAME test_soil_calibration COMMAND test_soil_calibration)

# Plant feature test (P1-3)
add_executable(test_plant_feature
    test_plant_feature.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ai/PlantFeature.c
)
add_test(NAME test_plant_feature COMMAND test_plant_feature)

# Plant log record test (P2-2)
add_executable(test_plant_log_record
    test_plant_log_record.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRecord.c
)
add_test(NAME test_plant_log_record COMMAND test_plant_log_record)

# Sensor plausibility test (P3-1)
add_executable(test_sensor_plausibility
    test_sensor_plausibility.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/sensors/SensorPlausibility.c
)
add_test(NAME test_sensor_plausibility COMMAND test_sensor_plausibility)

# Plant diagnosis test (P4-2)
add_executable(test_plant_diagnosis
    test_plant_diagnosis.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ai/PlantDiagnosis.c
)
add_test(NAME test_plant_diagnosis COMMAND test_plant_diagnosis)

# Plant stress barometer test (P4-3)
add_executable(test_plant_stress
    test_plant_stress.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ai/PlantStress.c
)
add_test(NAME test_plant_stress COMMAND test_plant_stress)

# LCD UI formatting test (P4-4)
add_executable(test_lcd_ui
    test_lcd_ui.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ui/LcdUi.c
)
add_test(NAME test_lcd_ui COMMAND test_lcd_ui)

# Watering policy test (P5-5)
add_executable(test_watering_policy
    test_watering_policy.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/app/WateringPolicy.c
)
add_test(NAME test_watering_policy COMMAND test_watering_policy)

# Watering response and soil degradation test (P6-1, P6-2)
add_executable(test_watering_response
    test_watering_response.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/ai/WateringResponse.c
)
add_test(NAME test_watering_response COMMAND test_watering_response)

# Demo mode test (P8-1)
add_executable(test_demo_mode
    test_demo_mode.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/app/DemoMode.c
)
add_test(NAME test_demo_mode COMMAND test_demo_mode)

# Time keeper test (P2-5)
add_executable(test_time_keeper
    test_time_keeper.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/app/TimeKeeper.c
)
add_test(NAME test_time_keeper COMMAND test_time_keeper)

# Console test (P2-4)
add_executable(test_console
    test_console.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/console/Console.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRecord.c
)
add_test(NAME test_console COMMAND test_console)

# Log queue test (P2-6)
add_executable(test_plant_log_queue
    test_plant_log_queue.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogQueue.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRecord.c
)
add_test(NAME test_plant_log_queue COMMAND test_plant_log_queue)

# FeRAM persistent ring buffer test (P2-3)
add_executable(test_plant_log_ring
    test_plant_log_ring.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRing.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/FramDriver.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRecord.c
)
add_test(NAME test_plant_log_ring COMMAND test_plant_log_ring)

# RTC test (P2-5)
add_executable(test_rtc
    test_rtc.c
    ../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/board/RtcRx4111.c
)
add_test(NAME test_rtc COMMAND test_rtc)







