import { HistorySampleRecord } from '../types';

/**
 * Fetch 10-minute downsampled telemetry history from ATOMS3 Lite / Server
 */
export async function fetchServerHistory(gatewayHost: string): Promise<HistorySampleRecord[]> {
  try {
    const res = await fetch(`http://${gatewayHost}/api/history`, {
      method: 'GET',
      headers: { 'Accept': 'application/json' },
      signal: AbortSignal.timeout(4000),
    });

    if (!res.ok) {
      throw new Error(`Server returned status ${res.status}`);
    }

    const data = await res.json();
    if (Array.isArray(data) && data.length > 0) {
      return data;
    }
  } catch (err) {
    console.warn('Server history fetch failed or unavailable, using fallback mock/cache:', err);
  }

  // If server is not reachable or empty, return realistic mock history for demonstration
  return getCachedOrMockHistory();
}

/**
 * Clear server history records
 */
export async function clearServerHistory(gatewayHost: string): Promise<boolean> {
  try {
    const res = await fetch(`http://${gatewayHost}/api/history/clear`, {
      method: 'POST',
      signal: AbortSignal.timeout(3000),
    });
    return res.ok;
  } catch (err) {
    console.error('Failed to clear server history:', err);
    return false;
  }
}

const LOCAL_STORAGE_KEY = 'plant_doctor_10min_history_cache';

function getCachedOrMockHistory(): HistorySampleRecord[] {
  try {
    const cached = localStorage.getItem(LOCAL_STORAGE_KEY);
    if (cached) {
      const parsed = JSON.parse(cached);
      if (Array.isArray(parsed) && parsed.length > 0) {
        return parsed;
      }
    }
  } catch (e) {
    console.warn(e);
  }

  const generated = generateMockHistorySamples(72); // 12 hours of 10-minute records
  try {
    localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(generated));
  } catch (e) {
    // ignore
  }
  return generated;
}

/**
 * Generate synthetic realistic 10-minute downsampled historical records
 * Simulating diurnal cycle (day/night temp, humidity, transpiration, watering event)
 */
export function generateMockHistorySamples(count: number = 72): HistorySampleRecord[] {
  const records: HistorySampleRecord[] = [];
  const now = Math.floor(Date.now() / 1000);
  const intervalSec = 600; // 10 minutes

  // Start from past to now
  const startTime = now - count * intervalSec;

  for (let i = 0; i < count; i++) {
    const t = startTime + i * intervalSec;
    const date = new Date(t * 1000);
    const hour = date.getHours() + date.getMinutes() / 60;

    // Diurnal temperature wave (peak around 14:00, trough around 05:00)
    const diurnal = Math.sin(((hour - 8) / 24) * 2 * Math.PI);
    const air_temp = Number((22.0 + diurnal * 4.5 + (Math.random() - 0.5) * 0.4).toFixed(2));
    const humidity = Number((60.0 - diurnal * 15.0 + (Math.random() - 0.5) * 2.0).toFixed(1));
    const lux = Math.max(0, Math.round(500 * Math.sin(((hour - 6) / 12) * Math.PI) + (Math.random() - 0.5) * 50));

    // Soil moisture slowly decreasing until simulated watering around 4 hours ago
    const hoursAgo = (now - t) / 3600;
    let soil_raw = 1750;
    let pump_on = false;
    let status = 'HEALTHY';
    let stress = 15;

    if (hoursAgo > 4.5 && hoursAgo < 6.0) {
      // Dry stress period before watering
      soil_raw = 2200 + Math.round((hoursAgo - 4.5) * 150);
      stress = Math.min(65, Math.round(40 + (hoursAgo - 4.5) * 15));
      status = 'DRY_STRESS';
    } else if (hoursAgo >= 4.0 && hoursAgo <= 4.5) {
      // Watering event!
      soil_raw = 1800;
      pump_on = i % 2 === 0;
      status = 'WATERING';
      stress = 20;
    } else if (hoursAgo < 4.0) {
      // Post-watering healthy state
      soil_raw = 1650 + Math.round((4.0 - hoursAgo) * 35);
      stress = 12 + Math.round(Math.random() * 6);
      status = 'HEALTHY';
    }

    // Leaf temp: active transpiration lowers leaf temp below air temp (healthy)
    let leaf_air_diff = -0.6;
    if (status === 'DRY_STRESS') {
      leaf_air_diff = +0.8; // Transpiration suppressed
    } else if (lux > 300) {
      leaf_air_diff = -1.2; // Vigorous transpiration
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
      ai_loss: Number((0.022 + Math.exp(-i / 20) * 0.03 + (Math.random() - 0.5) * 0.002).toFixed(4)),
      ai_phase: 2,
      ai_anomaly_score: stress > 50 ? 45 : 8,
      leaf_temp_rate: Number(((Math.random() - 0.5) * 0.3).toFixed(2)),
      soil_rate: status === 'WATERING' ? 120 : status === 'DRY_STRESS' ? -35 : -10,
    });
  }

  return records;
}

/**
 * Export history records to CSV file
 */
export function exportHistoryToCsv(records: HistorySampleRecord[], filename?: string): void {
  const headers = [
    'Timestamp',
    'DateTime (JST)',
    'Seq',
    'Stress (0-100)',
    'Status',
    'SoilTrend',
    'AirTemp (degC)',
    'Humidity (%)',
    'LeafTemp (degC)',
    'DeltaT (degC)',
    'SoilRaw (0-4095)',
    'Lux',
    'TankLiquid',
    'PumpOn',
    'AiTrainCount',
    'AiLoss',
    'AiPhase',
    'AiAnomalyScore',
  ];

  const rows = records.map((r) => {
    const d = new Date(r.timestamp * 1000);
    const dateStr = d.toLocaleString('ja-JP', {
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    });

    return [
      r.timestamp,
      `"${dateStr}"`,
      r.seq,
      r.stress,
      r.status,
      r.soil_trend,
      r.air_temp.toFixed(2),
      r.humidity.toFixed(1),
      r.leaf_temp.toFixed(2),
      r.leaf_air_diff.toFixed(2),
      r.soil_raw,
      r.lux,
      r.tank_liquid ? 1 : 0,
      r.pump_on ? 1 : 0,
      r.ai_train_count ?? 0,
      r.ai_loss?.toFixed(4) ?? '0.0000',
      r.ai_phase ?? 0,
      r.ai_anomaly_score ?? 0,
    ].join(',');
  });

  const csvContent = '\uFEFF' + [headers.join(','), ...rows].join('\r\n');
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.setAttribute('href', url);
  const name =
    filename ||
    `plant_doctor_10min_logs_${new Date().toISOString().slice(0, 10)}.csv`;
  link.setAttribute('download', name);
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
}
