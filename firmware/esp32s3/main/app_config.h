/** =================================================================*
 * @file   app_config.h
 * @brief  ATOMS3 Lite Gateway Configuration
 * ================================================================= */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Wi-Fi Configuration */
#define CONFIG_ESP_WIFI_SSID               "Fight Club"
#define CONFIG_ESP_WIFI_PASS               "soap1999"
#define CONFIG_ESP_MAXIMUM_RETRY           5
#define CONFIG_MDNS_HOST_NAME              "plant-doctor"

/* ATOMS3 Lite Hardware Pin Definitions */
#define ATOM_S3_RGB_LED_GPIO               35      /* WS2812B NeoPixel */
#define ATOM_S3_BUTTON_GPIO                41      /* Front button (active low) */

/* USB Host & FTDI Configuration */
#define FTDI_BAUDRATE                      115200
#define FTDI_DATA_BITS                     8
#define FTDI_STOP_BITS                     1
#define FTDI_PARITY                        0       /* None */
#define FTDI_CHANNEL                       1       /* Channel B for DT-EBML63Q2557 UARTF1 */

/* Polling & Communication Configuration */
#define SENSOR_POLL_INTERVAL_MS            1000    /* 1.0 second */
#define DIAGNOSIS_POLL_INTERVAL_MS         2000    /* 2.0 seconds */
#define UART_RESPONSE_TIMEOUT_MS           300     /* 300 ms wait for response */
#define UART_RX_BUFFER_SIZE                512

/* Telemetry Data Snapshot Structure */
typedef struct {
    uint32_t timestamp;                     /* UNIX time */
    uint32_t sampleSequence;                /* Sample counter */
    int16_t  airTemperatureCentiC;          /* 1/100 deg C */
    uint16_t relativeHumidityCentiPercent;  /* 1/100 % */
    int16_t  leafTemperatureCentiC;         /* 1/100 deg C */
    int16_t  leafAirDiffCentiC;             /* leaf - air */
    uint16_t soilMoistureRaw;               /* 0 - 4095 */
    uint16_t illuminanceRaw;                /* Raw lux measure */
    bool     tankLiquidDetected;            /* true = water exists */
    
    /* AI Diagnosis & Status */
    uint8_t  stressScore;                   /* 0 - 100 */
    char     status[32];                    /* e.g. "HEALTHY", "HEAT_STRESS", etc. */
    char     soilTrend[16];                 /* "STABLE", "DRYING", "WETTING", "DEGRADED" */
    
    /* Solist-AI™ Extended Metrics */
    uint16_t aiTrainCount;                  /* On-Device Learning step count */
    float    aiLoss;                        /* Reconstruction error (0.0000 - 1.0000) */
    uint8_t  aiPhase;                       /* 0: Profiling, 1: Stabilizing, 2: Monitoring */
    uint8_t  aiAnomalyScore;                /* 0 - 100 Anomaly deviation */
    float    leafTempRatePerHour;           /* deg C / hour */
    int16_t  soilMoistureRatePerHour;       /* permille / hour */

    bool     demoMode;
    bool     pumpOn;
    bool     valid;
    bool     ebml_connected;                /* true if DT-EBML communication is active */
    char     conn_type[16];                 /* "I2C" or "USB" */
} plant_telemetry_t;

#endif /* APP_CONFIG_H */

