/** =================================================================*
 * @file   PlantLog.c
 * @brief  植物ログ保存 (P2-6)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantLog.h"
#include "PlantLogQueue.h"
#include "PlantLogRecord.h"
#include "PlantDoctorConfig.h"
#include "SensorManager.h"
#include "PlantAi.h"
#include "TimeKeeper.h"
#include "PlantLogRing.h"
#include <string.h>

#define RECENT_LOG_MAX              (16U)

static PLANT_LOG_QUEUE s_logQueue;
static uint16_t s_periodicTicks;
static uint16_t s_recordSequence;
static uint8_t s_bootCount = 0U;
static PLANT_STATUS s_lastStatus;
static bool s_hasLastStatus;

static PLANT_LOG_RECORD s_recentRecords[RECENT_LOG_MAX];
static uint8_t s_recentHead;
static uint8_t s_recentCount;

/** =================================================================*
 * @brief  レコード共通フィールド設定
 * ================================================================= */
static void PlantLog_FillRecord(PLANT_LOG_RECORD *rec,
                                uint8_t recordType,
                                const PLANT_SENSOR_SNAPSHOT *snap,
                                PLANT_STATUS status,
                                uint8_t stressScore,
                                uint16_t eventPayload) {
    PlantLogRecord_Clear(rec);
    rec->timestampSeconds = TimeKeeper_GetUnixSeconds();
    rec->recordSequence = s_recordSequence;
    ++s_recordSequence;
    rec->recordType = recordType;
    rec->bootCount = s_bootCount;
    rec->plantStatus = (uint8_t)status;
    rec->stressScore = stressScore;
    rec->eventPayload = eventPayload;

    if (snap != NULL) {
        rec->validFlags = 0U;
        if (snap->soilMoistureValid) {
            rec->validFlags |= 0x01U;
        }
        if (snap->leafTemperatureValid) {
            rec->validFlags |= 0x02U;
        }
        if (snap->airTemperatureValid) {
            rec->validFlags |= 0x04U;
        }
        if (snap->barometricPressureValid) {
            rec->validFlags |= 0x08U;
        }
        if (snap->illuminanceValid) {
            rec->validFlags |= 0x10U;
        }
        if (snap->tankLiquidDetected) {
            rec->validFlags |= 0x20U;
        }

        rec->soilMoistureRaw = snap->soilMoistureRaw;
        rec->leafTemperatureCentiC = snap->leafTemperatureCentiC;
        rec->airTemperatureCentiC = snap->airTemperatureCentiC;
        rec->relativeHumidityCentiPercent = snap->relativeHumidityCentiPercent;
        rec->illuminanceRaw = snap->illuminanceRaw;
    }
}

/** =================================================================*
 * @brief  RAM直近保持バッファへの追加
 * ================================================================= */
static void PlantLog_StoreRecent(const PLANT_LOG_RECORD *rec) {
    s_recentRecords[s_recentHead] = *rec;
    s_recentHead = (uint8_t)((s_recentHead + 1U) % RECENT_LOG_MAX);
    if (s_recentCount < RECENT_LOG_MAX) {
        ++s_recentCount;
    }
}

/** =================================================================*
 * @brief  PlantLog_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PlantLog_Init(void) {
    PLANT_LOG_RECORD bootRec;

    PlantLogRing_Init();
    PlantLogQueue_Init(&s_logQueue);
    s_periodicTicks = PLANT_DOCTOR_LOG_PERIODIC_INTERVAL_TICKS;
    s_recordSequence = 0U;
    ++s_bootCount;
    s_hasLastStatus = false;
    s_recentHead = 0U;
    s_recentCount = 0U;

    /* 起動イベント (recordType = 2) をキューへ格納 */
    PlantLog_FillRecord(&bootRec, 2U, NULL, PLANT_STATUS_HEALTHY, 0xFFU, (uint16_t)s_bootCount);
    (void)PlantLogQueue_Enqueue(&s_logQueue, &bootRec);

    return true;
}

/** =================================================================*
 * @brief  PlantLog_Append処理
 * ================================================================= */
bool PlantLog_Append(const uint8_t *data, uint16_t size) {
    (void)data;
    (void)size;
    return false;
}

/** =================================================================*
 * @brief  PlantLog_Process10Ms処理
 * ================================================================= */
void PlantLog_Process10Ms(void) {
    PLANT_SENSOR_SNAPSHOT snap;
    bool hasSnap = SensorManager_GetLatest(&snap);
    PLANT_STATUS currentStatus = PlantAi_GetStatus();
    uint8_t stressScore = PlantAi_GetStressScore();

    /* 診断状態の変化検出 (recordType = 4) */
    if (s_hasLastStatus) {
        if (currentStatus != s_lastStatus) {
            PLANT_LOG_RECORD changeRec;
            PlantLog_FillRecord(&changeRec, 4U, hasSnap ? &snap : NULL,
                                currentStatus, stressScore, (uint16_t)s_lastStatus);
            (void)PlantLogQueue_Enqueue(&s_logQueue, &changeRec);
            s_lastStatus = currentStatus;
        }
    } else {
        s_lastStatus = currentStatus;
        s_hasLastStatus = true;
    }

    /* 60秒周期の定期レコード (recordType = 0) */
    if (s_periodicTicks > 0U) {
        --s_periodicTicks;
    } else {
        PLANT_LOG_RECORD periodicRec;
        s_periodicTicks = PLANT_DOCTOR_LOG_PERIODIC_INTERVAL_TICKS;
        PlantLog_FillRecord(&periodicRec, 0U, hasSnap ? &snap : NULL,
                            currentStatus, stressScore, 0U);
        (void)PlantLogQueue_Enqueue(&s_logQueue, &periodicRec);
    }

    /* キュー処理 (1 tickあたり1件をRAM直近バッファおよびFeRAMリングバッファへコミット) */
    if (!PlantLogQueue_IsEmpty(&s_logQueue)) {
        PLANT_LOG_RECORD rec;
        if (PlantLogQueue_Dequeue(&s_logQueue, &rec)) {
            PlantLog_StoreRecent(&rec);
            (void)PlantLogRing_Append(&rec);
        }
    }
}

/** =================================================================*
 * @brief  PlantLog_NotifyWatering処理
 * ================================================================= */
void PlantLog_NotifyWatering(const PUMP_WATERING_EVENT *event) {
    PLANT_SENSOR_SNAPSHOT snap;
    bool hasSnap = SensorManager_GetLatest(&snap);
    PLANT_STATUS currentStatus = PlantAi_GetStatus();
    uint8_t stressScore = PlantAi_GetStressScore();
    PLANT_LOG_RECORD rec;

    PlantLog_FillRecord(&rec, 1U, hasSnap ? &snap : NULL,
                        currentStatus, stressScore,
                        event ? (uint16_t)event->onDurationTicks : 0U);
    (void)PlantLogQueue_Enqueue(&s_logQueue, &rec);
}

/** =================================================================*
 * @brief  PlantLog_NotifyError処理
 * ================================================================= */
void PlantLog_NotifyError(PLANT_DOCTOR_ERROR error) {
    PLANT_SENSOR_SNAPSHOT snap;
    bool hasSnap = SensorManager_GetLatest(&snap);
    PLANT_LOG_RECORD rec;

    PlantLog_FillRecord(&rec, 3U, hasSnap ? &snap : NULL,
                        PLANT_STATUS_SENSOR_ERROR, 0xFFU, (uint16_t)error);
    (void)PlantLogQueue_Enqueue(&s_logQueue, &rec);
}

/** =================================================================*
 * @brief  PlantLog_GetBootCount処理
 * ================================================================= */
uint8_t PlantLog_GetBootCount(void) {
    return s_bootCount;
}

/** =================================================================*
 * @brief  PlantLog_GetRecordCount処理
 * ================================================================= */
uint16_t PlantLog_GetRecordCount(void) {
    uint32_t ringCount = PlantLogRing_GetCount();
    if (ringCount > 0U) {
        return (ringCount > 65535UL) ? 65535U : (uint16_t)ringCount;
    }
    return (uint16_t)s_recentCount;
}

/** =================================================================*
 * @brief  PlantLog_ReadRecord処理
 * ================================================================= */
bool PlantLog_ReadRecord(uint16_t index, PLANT_LOG_RECORD *record) {
    uint8_t startIdx;
    uint8_t target;

    if (record == NULL) {
        return false;
    }

    if (PlantLogRing_GetCount() > 0U) {
        return PlantLogRing_ReadRecord((uint32_t)index, record);
    }

    if (index >= s_recentCount) {
        return false;
    }

    /* 最古のレコードから順にindex番目を返す */
    if (s_recentCount < RECENT_LOG_MAX) {
        startIdx = 0U;
    } else {
        startIdx = s_recentHead;
    }
    target = (uint8_t)((startIdx + index) % RECENT_LOG_MAX);
    *record = s_recentRecords[target];
    return true;
}

/** =================================================================*
 * @brief  PlantLog_Erase処理
 * ================================================================= */
bool PlantLog_Erase(void) {
    PlantLogRing_Clear();
    PlantLogQueue_Init(&s_logQueue);
    s_recentHead = 0U;
    s_recentCount = 0U;
    return true;
}
