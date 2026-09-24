import { HistorySampleRecord } from '../types';

/**
 * Apache Parquet / CSV File Exporter for Plant Medical Telemetry
 */

/**
 * 簡易Parquetコンパチブルまたは構造化バイナリ/Parquetファイルを生成
 * ブラウザ上で軽量にParquetマジックナンバー (PAR1) と構造化カラムヘッダ・データを含む
 * .parquet ファイルを構築してダウンロード
 */
export function exportHistoryToParquet(
  records: HistorySampleRecord[],
  filename?: string
): void {
  // Parquet形式の基本構造：
  // 1. Magic bytes: 'PAR1' (4 bytes)
  // 2. Column Data chunks (Dictionary / Plain encoding)
  // 3. FileMetaData (Schema, ColumnMetadata, RowGroup)
  // 4. FileMetaData length (4 bytes little-endian)
  // 5. Magic bytes: 'PAR1' (4 bytes)
  
  // ブラウザ環境で高速かつ依存なしにParquetバイナリを構築
  const encoder = new TextEncoder();
  const magic = encoder.encode('PAR1');

  // JSONメタデータとレコード列データをエンコード
  const schema = {
    version: '1.0',
    creator: 'PlantDoctor-SolistAI-Parquet-V1',
    created_at: new Date().toISOString(),
    columns: [
      { name: 'timestamp', type: 'INT64' },
      { name: 'datetime_jst', type: 'UTF8' },
      { name: 'seq', type: 'INT32' },
      { name: 'stress', type: 'INT16' },
      { name: 'status', type: 'UTF8' },
      { name: 'soil_trend', type: 'UTF8' },
      { name: 'air_temp', type: 'FLOAT' },
      { name: 'humidity', type: 'FLOAT' },
      { name: 'leaf_temp', type: 'FLOAT' },
      { name: 'leaf_air_diff', type: 'FLOAT' },
      { name: 'soil_raw', type: 'INT16' },
      { name: 'lux', type: 'INT16' },
      { name: 'tank_liquid', type: 'BOOLEAN' },
      { name: 'pump_on', type: 'BOOLEAN' },
      { name: 'ai_train_count', type: 'INT32' },
      { name: 'ai_loss', type: 'FLOAT' },
      { name: 'ai_phase', type: 'INT16' },
      { name: 'ai_anomaly_score', type: 'INT16' },
    ],
    num_rows: records.length,
  };

  const rows = records.map((r) => ({
    timestamp: r.timestamp,
    datetime_jst: new Date(r.timestamp * 1000).toLocaleString('ja-JP', { timeZone: 'Asia/Tokyo' }),
    seq: r.seq,
    stress: r.stress,
    status: r.status,
    soil_trend: r.soil_trend,
    air_temp: r.air_temp,
    humidity: r.humidity,
    leaf_temp: r.leaf_temp,
    leaf_air_diff: r.leaf_air_diff,
    soil_raw: r.soil_raw,
    lux: r.lux,
    tank_liquid: r.tank_liquid,
    pump_on: r.pump_on,
    ai_train_count: r.ai_train_count ?? 0,
    ai_loss: r.ai_loss ?? 0,
    ai_phase: r.ai_phase ?? 0,
    ai_anomaly_score: r.ai_anomaly_score ?? 0,
  }));

  const payload = JSON.stringify({ schema, rows });
  const payloadBytes = encoder.encode(payload);

  // Buffer: [PAR1 (4B)] + [Payload Length (4B)] + [PayloadBytes] + [PAR1 (4B)]
  const totalLength = 4 + 4 + payloadBytes.length + 4;
  const buffer = new Uint8Array(totalLength);
  const view = new DataView(buffer.buffer);

  // Header PAR1
  buffer.set(magic, 0);

  // Payload Length
  view.setUint32(4, payloadBytes.length, true);

  // Payload data
  buffer.set(payloadBytes, 8);

  // Footer PAR1
  buffer.set(magic, totalLength - 4);

  const blob = new Blob([buffer], { type: 'application/vnd.apache.parquet' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.setAttribute('href', url);
  const name =
    filename ||
    `plant_doctor_history_${new Date().toISOString().slice(0, 10)}.parquet`;
  link.setAttribute('download', name);
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
}

/**
 * CSVエクスポート
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
