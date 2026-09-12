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

typedef enum {
    SENSOR_HEALTH_OK = 0,                                   /* 正常 */
    SENSOR_HEALTH_NO_COMMUNICATION,                         /* 通信不能 */
    SENSOR_HEALTH_OUT_OF_RANGE,                             /* 物理範囲外 */
    SENSOR_HEALTH_INCONSISTENT                              /* 他センサーと矛盾 */
} SENSOR_HEALTH;

typedef struct {
    SENSOR_HEALTH soilHealth;                               /* 土壌水分状態 */
    SENSOR_HEALTH leafHealth;                               /* 葉温状態 */
    SENSOR_HEALTH airHumHealth;                             /* 気温・湿度状態 */
    SENSOR_HEALTH luxHealth;                                /* 照度状態 */
} SENSOR_HEALTH_REPORT;

typedef enum {
    PLANT_STATUS_HEALTHY = 0,                               /* Healthy */
    PLANT_STATUS_DRY_STRESS,                                /* Dry Stress */
    PLANT_STATUS_HEAT_STRESS,                               /* Heat Stress */
    PLANT_STATUS_LOW_LIGHT,                                 /* 日照不足 */
    PLANT_STATUS_ROOT_UPTAKE,                               /* 根の吸水不良 */
    PLANT_STATUS_WATERING,                                  /* Watering */
    PLANT_STATUS_WATERING_FAILED,                           /* Watering Failed */
    PLANT_STATUS_SOIL_DEGRADATION,                          /* 土壌の劣化 */
    PLANT_STATUS_WARNING,                                   /* Warning */
    PLANT_STATUS_SENSOR_ERROR,                              /* Sensor Error */
    PLANT_STATUS_LEARNING                                   /* 学習未完了 */
} PLANT_STATUS;

typedef enum {
    SOIL_TREND_UNKNOWN = 0,                                 /* 履歴不足 */
    SOIL_TREND_STABLE,                                      /* Stable */
    SOIL_TREND_DRY,                                         /* Dry */
    SOIL_TREND_WET,                                         /* Wet */
    SOIL_TREND_DEGRADATION                                  /* Degradation */
} SOIL_TREND;

typedef enum {
    WATERING_RESPONSE_IDLE = 0,                             /* 給水待ち */
    WATERING_RESPONSE_MEASURING,                            /* 給水後の応答測定中 */
    WATERING_RESPONSE_OK,                                   /* 十分な回復を確認 */
    WATERING_RESPONSE_FAILED                                /* 水分が回復しない */
} WATERING_RESPONSE;

#define PLANT_STRESS_UNKNOWN               (0xFFU)

#endif /* PLANT_DOCTOR_STATUS_H */
