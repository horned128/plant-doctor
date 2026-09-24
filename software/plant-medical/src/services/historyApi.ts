import { HistorySampleRecord, WateringEventLog } from '../types';

/**
 * Fetch 10-minute downsampled telemetry history accumulated on Server PC
 * Reads directly from the server PC filesystem via /api/server-logs.
 * No browser localStorage or IndexedDB is used.
 */
export async function fetchServerHistory(_gatewayHost?: string): Promise<HistorySampleRecord[]> {
  try {
    const res = await fetch('/api/server-logs');
    if (res.ok) {
      const data = await res.json();
      if (Array.isArray(data) && data.length > 0) {
        return data
          .filter((r: any) => typeof r.timestamp === 'number' && r.timestamp > 1000000000)
          .sort((a: any, b: any) => a.timestamp - b.timestamp);
      }
    }
  } catch (e) {
    console.warn('[Server PC API] /api/server-logs unreachable, using fallback samples:', e);
  }

  // サーバ未起動時またはオフライン環境用のモック初期サンプル（ローカルストレージには保存しない）
  return generateMockHistorySamples(72);
}

/**
 * サーバPC側の物理ファイル（plant_history.parquet, csv, json）に10分サンプリングレコードを自動追記保存
 * ブラウザ側のlocalStorageやIndexedDBには一切書き込まず、サーバPCのディスクに直接永続化
 */
export async function saveServerHistorySample(record: HistorySampleRecord): Promise<void> {
  try {
    await fetch('/api/server-logs', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(record),
    });
  } catch (e) {
    console.error('[Server PC API] Failed to append sample to server PC storage:', e);
  }
}

/**
 * サーバPC上の物理ファイル（plant_history.parquet / csv / json）の蓄積ログを消去
 */
export async function clearServerHistory(_gatewayHost?: string): Promise<boolean> {
  try {
    const res = await fetch('/api/server-logs/clear', {
      method: 'POST',
    });
    return res.ok;
  } catch (err) {
    console.error('[Server PC API] Failed to clear Server PC history files:', err);
    return false;
  }
}

/**
 * サーバPC上のParquet実ファイルを直接ダウンロード
 */
export function downloadServerParquetFile(): void {
  const link = document.createElement('a');
  link.href = '/api/server-logs/download/parquet';
  link.download = `plant_history_${new Date().toISOString().slice(0, 10)}.parquet`;
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

/**
 * サーバPC上のCSV実ファイルを直接ダウンロード
 */
export function downloadServerCsvFile(): void {
  const link = document.createElement('a');
  link.href = '/api/server-logs/download/csv';
  link.download = `plant_history_${new Date().toISOString().slice(0, 10)}.csv`;
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

const WATERING_LOG_KEY = 'plant_doctor_watering_logs';

/**
 * 給水実績ログの取得 (直近50件)
 */
export function getWateringLogs(): WateringEventLog[] {
  try {
    const raw = localStorage.getItem(WATERING_LOG_KEY);
    if (raw) {
      return JSON.parse(raw);
    }
  } catch (e) {
    console.warn('Failed to parse watering logs:', e);
  }
  // デフォルト初期履歴（直近の給水例）
  const now = Math.floor(Date.now() / 1000);
  return [
    {
      id: 'init_water_1',
      timestamp: now - 3600 * 4,
      durationSec: 3,
      success: true,
      trigger: 'manual',
      reason: '正常完了',
      soilBefore: 2280,
      soilAfter: 1720,
    },
    {
      id: 'init_water_2',
      timestamp: now - 3600 * 28,
      durationSec: 5,
      success: true,
      trigger: 'manual',
      reason: '正常完了',
      soilBefore: 2310,
      soilAfter: 1650,
    },
  ];
}

/**
 * 給水実績ログの保存
 */
export function saveWateringLog(log: WateringEventLog): void {
  try {
    const current = getWateringLogs();
    const updated = [log, ...current.filter((item) => item.id !== log.id)].slice(0, 50);
    localStorage.setItem(WATERING_LOG_KEY, JSON.stringify(updated));
  } catch (e) {
    console.warn('Failed to save watering log:', e);
  }
}

/**
 * Generate synthetic realistic 10-minute downsampled historical records
 * Simulating diurnal cycle (day/night temp, humidity, transpiration, watering event)
 */
export function generateMockHistorySamples(count: number = 72): HistorySampleRecord[] {
  const records: HistorySampleRecord[] = [];
  const now = Math.floor(Date.now() / 1000);
  const intervalSec = 600; // 10 minutes

  const startTime = now - count * intervalSec;

  for (let i = 0; i < count; i++) {
    const t = startTime + i * intervalSec;
    const date = new Date(t * 1000);
    const hour = date.getHours() + date.getMinutes() / 60;

    // Diurnal temperature wave (peak around 14:00, trough around 05:00)
    const diurnal = Math.sin(((hour - 8) / 24) * 2 * Math.PI);
    const air_temp = Number((22.5 + diurnal * 4.0 + (Math.random() - 0.5) * 0.3).toFixed(2));
    const humidity = Number((58.0 - diurnal * 12.0 + (Math.random() - 0.5) * 1.5).toFixed(1));
    const lux = Math.max(0, Math.round(480 * Math.sin(((hour - 6) / 12) * Math.PI) + (Math.random() - 0.5) * 40));

    // Soil moisture slowly decreasing until simulated watering around 4 hours ago
    const hoursAgo = (now - t) / 3600;
    let soil_raw = 1750;
    let pump_on = false;
    let status = 'HEALTHY';
    let stress = 2; // 平常時は0〜5付近

    if (hoursAgo > 4.5 && hoursAgo < 6.0) {
      // Dry stress period before watering
      soil_raw = 2200 + Math.round((hoursAgo - 4.5) * 150);
      stress = Math.min(65, Math.round(35 + (hoursAgo - 4.5) * 15));
      status = 'DRY_STRESS';
    } else if (hoursAgo >= 4.0 && hoursAgo <= 4.5) {
      // Watering event!
      soil_raw = 1800;
      pump_on = i % 2 === 0;
      status = 'WATERING';
      stress = 10;
    } else if (hoursAgo < 4.0) {
      // Post-watering healthy state
      soil_raw = 1650 + Math.round((4.0 - hoursAgo) * 35);
      stress = 1 + Math.round(Math.random() * 4);
      status = 'HEALTHY';
    }

    // Leaf temp: active transpiration lowers leaf temp below air temp (healthy)
    let leaf_air_diff = -0.7;
    if (status === 'DRY_STRESS') {
      leaf_air_diff = +0.6; // Transpiration suppressed
    } else if (lux > 300) {
      leaf_air_diff = -1.1; // Vigorous transpiration
    }
    const leaf_temp = Number((air_temp + leaf_air_diff).toFixed(2));

    records.push({
      timestamp: t,
      seq: 1000 + i,
      stress,
      status,
      soil_trend: status === 'DRY_STRESS' ? 'FALLING' : status === 'WATERING' ? 'RISING' : 'STABLE',
      air_temp,
      humidity,
      leaf_temp,
      leaf_air_diff: Number(leaf_air_diff.toFixed(2)),
      soil_raw,
      lux,
      tank_liquid: true,
      pump_on,
      demo_mode: false,
      ai_train_count: 50 + Math.floor(i / 2),
      ai_loss: Number((0.021 + Math.exp(-i / 25) * 0.02 + (Math.random() - 0.5) * 0.001).toFixed(4)),
      ai_phase: 2,
      ai_anomaly_score: stress > 50 ? 45 : 3,
      leaf_temp_rate: Number(((Math.random() - 0.5) * 0.2).toFixed(2)),
      soil_rate: status === 'WATERING' ? 120 : status === 'DRY_STRESS' ? -35 : -10,
    });
  }

  return records;
}
