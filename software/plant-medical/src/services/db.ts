import { TelemetryData, TelemetryRecord } from '../types';

const DB_NAME = 'PlantMedicalDB';
const DB_VERSION = 1;
const STORE_NAME = 'telemetry_logs';

let dbInstance: IDBDatabase | null = null;
let pendingRecords: TelemetryRecord[] = [];
let flushTimeout: any = null;

/**
 * Initialize IndexedDB instance
 */
export async function getDb(): Promise<IDBDatabase> {
  if (dbInstance) return dbInstance;

  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION);

    request.onupgradeneeded = (event) => {
      const db = (event.target as IDBOpenDBRequest).result;
      if (!db.objectStoreNames.contains(STORE_NAME)) {
        const store = db.createObjectStore(STORE_NAME, {
          keyPath: 'id',
          autoIncrement: true,
        });
        store.createIndex('recordTime', 'recordTime', { unique: false });
        store.createIndex('seq', 'seq', { unique: false });
      }
    };

    request.onsuccess = (event) => {
      dbInstance = (event.target as IDBOpenDBRequest).result;
      resolve(dbInstance);
    };

    request.onerror = (event) => {
      reject((event.target as IDBOpenDBRequest).error);
    };
  });
}

/**
 * Buffer a telemetry record and flush to IndexedDB
 */
export function queueTelemetryRecord(data: TelemetryData): void {
  const record: TelemetryRecord = {
    ...data,
    recordTime: Date.now(),
  };

  pendingRecords.push(record);

  if (!flushTimeout) {
    flushTimeout = setTimeout(() => {
      flushPendingRecords();
    }, 1000); // Flush once per second
  }
}

/**
 * Flush pending in-memory records to IndexedDB in a single transaction
 */
export async function flushPendingRecords(): Promise<void> {
  flushTimeout = null;
  if (pendingRecords.length === 0) return;

  const recordsToWrite = [...pendingRecords];
  pendingRecords = [];

  try {
    const db = await getDb();
    const tx = db.transaction(STORE_NAME, 'readwrite');
    const store = tx.objectStore(STORE_NAME);

    for (const record of recordsToWrite) {
      store.add(record);
    }

    await new Promise<void>((resolve, reject) => {
      tx.oncomplete = () => resolve();
      tx.onerror = () => reject(tx.error);
    });
  } catch (err) {
    console.error('Failed to flush telemetry records to IndexedDB:', err);
    // Put back records on failure to avoid data loss
    pendingRecords = [...recordsToWrite, ...pendingRecords];
  }
}

/**
 * Query records within a timestamp range, downsampling if points exceed maxPoints
 */
export async function getTelemetryRange(
  startTime: number,
  endTime: number,
  maxPoints: number = 300
): Promise<TelemetryRecord[]> {
  // Ensure any pending records are written before querying
  await flushPendingRecords();

  const db = await getDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readonly');
    const store = tx.objectStore(STORE_NAME);
    const index = store.index('recordTime');
    const range = IDBKeyRange.bound(startTime, endTime);
    const request = index.getAll(range);

    request.onsuccess = () => {
      const records: TelemetryRecord[] = request.result || [];
      if (records.length <= maxPoints) {
        resolve(records);
        return;
      }

      // Uniform downsampling
      const step = records.length / maxPoints;
      const sampled: TelemetryRecord[] = [];
      for (let i = 0; i < maxPoints; i++) {
        const index = Math.min(Math.floor(i * step), records.length - 1);
        sampled.push(records[index]);
      }
      // Always include the latest record
      if (sampled[sampled.length - 1] !== records[records.length - 1]) {
        sampled[sampled.length - 1] = records[records.length - 1];
      }
      resolve(sampled);
    };

    request.onerror = () => {
      reject(request.error);
    };
  });
}

/**
 * Get database statistics (total count, earliest/latest timestamps, estimated size)
 */
export async function getDatabaseStats(): Promise<{
  totalRecords: number;
  estimatedSizeMb: number;
  earliestTime?: number;
  latestTime?: number;
}> {
  await flushPendingRecords();
  const db = await getDb();

  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readonly');
    const store = tx.objectStore(STORE_NAME);
    const countRequest = store.count();

    countRequest.onsuccess = () => {
      const totalRecords = countRequest.result;
      if (totalRecords === 0) {
        resolve({
          totalRecords: 0,
          estimatedSizeMb: 0,
        });
        return;
      }

      // Estimate ~200 bytes per JSON record in IndexedDB
      const estimatedSizeMb = Number(((totalRecords * 200) / (1024 * 1024)).toFixed(2));

      // Get earliest and latest timestamp using cursor
      const index = store.index('recordTime');
      const firstReq = index.openCursor(null, 'next');

      firstReq.onsuccess = () => {
        const firstCursor = firstReq.result;
        const earliestTime = firstCursor ? (firstCursor.value as TelemetryRecord).recordTime : undefined;

        const lastReq = index.openCursor(null, 'prev');
        lastReq.onsuccess = () => {
          const lastCursor = lastReq.result;
          const latestTime = lastCursor ? (lastCursor.value as TelemetryRecord).recordTime : undefined;

          resolve({
            totalRecords,
            estimatedSizeMb,
            earliestTime,
            latestTime,
          });
        };
        lastReq.onerror = () => reject(lastReq.error);
      };
      firstReq.onerror = () => reject(firstReq.error);
    };

    countRequest.onerror = () => {
      reject(countRequest.error);
    };
  });
}

/**
 * Clear all records in the database
 */
export async function clearAllRecords(): Promise<void> {
  pendingRecords = [];
  if (flushTimeout) {
    clearTimeout(flushTimeout);
    flushTimeout = null;
  }

  const db = await getDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readwrite');
    const store = tx.objectStore(STORE_NAME);
    const req = store.clear();

    req.onsuccess = () => resolve();
    req.onerror = () => reject(req.error);
  });
}

/**
 * Export all records as CSV string
 */
export async function exportAllAsCsv(): Promise<string> {
  await flushPendingRecords();
  const db = await getDb();

  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readonly');
    const store = tx.objectStore(STORE_NAME);
    const req = store.getAll();

    req.onsuccess = () => {
      const records: TelemetryRecord[] = req.result || [];
      const headers = [
        'RecordTimestamp',
        'DateTime',
        'Seq',
        'Stress',
        'Status',
        'SoilTrend',
        'AirTemp(C)',
        'Humidity(%)',
        'LeafTemp(C)',
        'LeafAirDiff(C)',
        'SoilRaw',
        'Lux',
        'TankLiquid',
        'PumpOn',
        'AiTrainCount',
        'AiLoss',
        'AiPhase',
        'AiAnomalyScore',
        'LeafTempRate(C/h)',
        'SoilRate(/h)',
      ];

      const rows = records.map((r) => [
        r.recordTime,
        new Date(r.recordTime).toISOString(),
        r.seq,
        r.stress,
        r.status,
        r.soil_trend,
        r.air_temp?.toFixed(2),
        r.humidity?.toFixed(2),
        r.leaf_temp?.toFixed(2),
        r.leaf_air_diff?.toFixed(2),
        r.soil_raw,
        r.lux,
        r.tank_liquid ? 1 : 0,
        r.pump_on ? 1 : 0,
        r.ai_train_count ?? 0,
        r.ai_loss?.toFixed(4) ?? '0.0000',
        r.ai_phase ?? 0,
        r.ai_anomaly_score ?? 0,
        r.leaf_temp_rate?.toFixed(2) ?? '0.00',
        r.soil_rate?.toFixed(2) ?? '0.00',
      ]);

      const csv = [headers.join(','), ...rows.map((row) => row.join(','))].join('\n');
      resolve(csv);
    };

    req.onerror = () => reject(req.error);
  });
}
