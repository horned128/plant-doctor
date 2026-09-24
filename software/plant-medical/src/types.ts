export interface TelemetryData {
  type: string;
  timestamp: number;
  seq: number;
  stress: number;
  status: string;
  soil_trend: string;
  air_temp: number;
  humidity: number;
  leaf_temp: number;
  leaf_air_diff: number;
  soil_raw: number;
  lux: number;
  tank_liquid: boolean;
  pump_on: boolean;
  demo_mode: boolean;
  valid: boolean;
  ebml_connected?: boolean;
  conn_type?: string;

  /* Solist-AI™ On-Device Learning Metrics */
  ai_train_count?: number;
  ai_loss?: number;
  ai_phase?: number; /* 0: Profiling, 1: Stabilizing, 2: Monitoring */
  ai_anomaly_score?: number;
  leaf_temp_rate?: number;
  soil_rate?: number;
}

export interface TelemetryRecord extends TelemetryData {
  id?: number;
  recordTime: number; /* epoch ms in browser */
}

export interface HistorySampleRecord {
  timestamp: number; // unix seconds from server
  seq: number;
  stress: number;
  status: string;
  soil_trend: string;
  air_temp: number;
  humidity: number;
  leaf_temp: number;
  leaf_air_diff: number;
  soil_raw: number;
  lux: number;
  tank_liquid: boolean;
  pump_on: boolean;
  demo_mode: boolean;
  ai_train_count?: number;
  ai_loss?: number;
  ai_phase?: number;
  ai_anomaly_score?: number;
  leaf_temp_rate?: number;
  soil_rate?: number;
}

export type ConnectionState = 'connected' | 'connecting' | 'disconnected';

export type HistoryRange = '10m' | '1h' | '6h' | '24h' | '7d' | 'all';

export type ActiveTab = 'charts' | 'logs' | 'bio' | 'ai';

export type TimeResolution = '1h' | '24h' | '7d' | '30d' | 'custom';

export interface CustomDateRange {
  startDate: string; // YYYY-MM-DD
  startHour: number; // 0-23
  endDate: string;   // YYYY-MM-DD
  endHour: number;   // 0-23
  singleDayOnly?: boolean; // 1日単位（開始日のみ）
}

export interface WateringEventLog {
  id: string;
  timestamp: number; // unix seconds
  durationSec: number;
  success: boolean;
  trigger: 'manual' | 'auto';
  reason?: string;
  soilBefore?: number;
  soilAfter?: number;
}

export type PlantSpeciesType = 'spathiphyllum' | 'pothos' | 'custom';

export interface PlantThresholds {
  soilMinRaw: number;      // 乾燥側 (Raw値高い)
  soilMaxRaw: number;      // 湿潤側 (Raw値低い)
  tempMin: number;         // ℃
  tempMax: number;         // ℃
  luxMin: number;          // 好適下限 Lux
  luxMax: number;          // 好適上限 Lux
  wiltSensitivity: number; // 0-100 (水切れ・しおれ感度)
}

export interface PlantProfile {
  id: string;
  name: string;            // ユーザーによる愛称
  species: PlantSpeciesType;
  speciesNameJa: string;   // 表示名
  description: string;
  thresholds: PlantThresholds;
  baselineFeatures: number[]; // 8次元基準モデル (0-100 normalized)
  baselineLoss: number;
  learnedSamples: number;
  updatedAt: number;
}

export interface DatabaseStats {
  totalRecords: number;
  estimatedSizeMb: number;
  earliestTime?: number;
  latestTime?: number;
  recording: boolean;
}
