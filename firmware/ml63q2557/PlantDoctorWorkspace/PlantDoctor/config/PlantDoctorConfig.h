/** =================================================================*
 * @file   PlantDoctorConfig.h
 * @brief  アプリケーション設定
 * ================================================================= */
#ifndef PLANT_DOCTOR_CONFIG_H
#define PLANT_DOCTOR_CONFIG_H

#define PLANT_DOCTOR_TICK_MS               (10U)
#define PLANT_DOCTOR_LED_BLINK_TICKS       (100U)
#define PLANT_DOCTOR_ERROR_BLINK_TICKS     (25U)
#define PLANT_DOCTOR_SELF_TEST_TICKS       (50U)
#define PLANT_DOCTOR_SENSOR_SAMPLE_TICKS   (100U)
#define PLANT_DOCTOR_SENSOR_DISPLAY_TICKS  (500U)
#define PLANT_DOCTOR_LCD_RECOVERY_INTERVAL_TICKS (100U)
#define PLANT_DOCTOR_LCD_RECOVERY_MAX_ATTEMPTS   (3U)
#define PLANT_DOCTOR_SWITCH_DEBOUNCE_POLLS (2U)
#define PLANT_DOCTOR_LCD_TIMEOUT_LOOPS     (50000UL)
#define PLANT_DOCTOR_I2C_TIMEOUT_LOOPS     (50000UL)
#define PLANT_DOCTOR_TIMER_START_TIMEOUT_LOOPS (480000UL)
#define PLANT_DOCTOR_PUMP_MAX_ON_TICKS     (200U)
#define PLANT_DOCTOR_PUMP_COOLDOWN_TICKS   (300U)
#define PLANT_DOCTOR_PUMP_REQUIRE_LIQUID   (false)

#endif /* PLANT_DOCTOR_CONFIG_H */
