/** =================================================================*
 * @file   PlantDoctorStatus.h
 * @brief  アプリケーション状態型
 * ================================================================= */
#ifndef PLANT_DOCTOR_STATUS_H
#define PLANT_DOCTOR_STATUS_H

typedef enum {
    PLANT_DOCTOR_ERROR_NONE = 0,
    PLANT_DOCTOR_ERROR_POWER,
    PLANT_DOCTOR_ERROR_TIMER,
    PLANT_DOCTOR_ERROR_SWITCH,
    PLANT_DOCTOR_ERROR_LCD_INIT,
    PLANT_DOCTOR_ERROR_LCD_IO,
    PLANT_DOCTOR_ERROR_TICK_OVERFLOW,
    PLANT_DOCTOR_ERROR_SENSOR_INTERFACE,
    PLANT_DOCTOR_ERROR_STORAGE_INTERFACE
} PLANT_DOCTOR_ERROR;

#endif /* PLANT_DOCTOR_STATUS_H */
