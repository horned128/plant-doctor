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

export type ConnectionState = 'connected' | 'connecting' | 'disconnected';

export type HistoryRange = '10m' | '1h' | '6h' | '24h' | 'all';

export interface DatabaseStats {
  totalRecords: number;
  estimatedSizeMb: number;
  earliestTime?: number;
  latestTime?: number;
  recording: boolean;
}
