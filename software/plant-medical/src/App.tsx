import { useState, useEffect, useRef, useMemo, useCallback } from 'react';
import {
  TelemetryData,
  TelemetryRecord,
  ConnectionState,
  HistoryRange,
  DatabaseStats,
} from './types';
import { Header } from './components/Header';
import { StressMeter } from './components/StressMeter';
import { DiagnosisCard } from './components/DiagnosisCard';
import { EnvironmentPanel } from './components/EnvironmentPanel';
import { WateringControl } from './components/WateringControl';
import { TimeSeriesChart } from './components/TimeSeriesChart';
import { SolistAiStatusCard } from './components/SolistAiStatusCard';
import { SolistAiLossChart } from './components/SolistAiLossChart';
import { SolistAiFeatureRadar } from './components/SolistAiFeatureRadar';
import { HistoryToolbar } from './components/HistoryToolbar';
import {
  queueTelemetryRecord,
  getTelemetryRange,
  getDatabaseStats,
  clearAllRecords,
  exportAllAsCsv,
} from './services/db';

const MAX_CHART_POINTS = 60; // 60 seconds for live view

export default function App() {
  const [gatewayHost, setGatewayHost] = useState('plant-doctor.local');
  const [connState, setConnState] = useState<ConnectionState>('connecting');

  // Live telemetry state
  const [telemetry, setTelemetry] = useState<TelemetryData>({
    type: 'telemetry',
    timestamp: 0,
    seq: 0,
    stress: 18,
    status: 'HEALTHY',
    soil_trend: 'STABLE',
    air_temp: 24.5,
    humidity: 55.0,
    leaf_temp: 24.0,
    leaf_air_diff: -0.5,
    soil_raw: 1850,
    lux: 450,
    tank_liquid: true,
    pump_on: false,
    demo_mode: false,
    valid: true,
    ai_train_count: 52,
    ai_loss: 0.0245,
    ai_phase: 2,
    ai_anomaly_score: 12,
    leaf_temp_rate: -0.2,
    soil_rate: -15.0,
  });

  // Short live rolling history (60 sec)
  const [liveHistory, setLiveHistory] = useState<{
    labels: string[];
    airTemps: number[];
    leafTemps: number[];
    soilRaws: number[];
    losses: number[];
  }>({
    labels: [],
    airTemps: [],
    leafTemps: [],
    soilRaws: [],
    losses: [],
  });

  // History & Storage state
  const [mode, setMode] = useState<'live' | 'history'>('live');
  const [historyRange, setHistoryRange] = useState<HistoryRange>('1h');
  const [isRecording, setIsRecording] = useState<boolean>(true);
  const [dbStats, setDbStats] = useState<DatabaseStats>({
    totalRecords: 0,
    estimatedSizeMb: 0,
    recording: true,
  });
  const [historyRecords, setHistoryRecords] = useState<TelemetryRecord[]>([]);
  const [scrubberIndex, setScrubberIndex] = useState<number>(0);

  const wsRef = useRef<WebSocket | null>(null);
  const isRecordingRef = useRef<boolean>(true);
  isRecordingRef.current = isRecording;

  // Refresh DB stats periodically
  const refreshDbStats = useCallback(async () => {
    try {
      const stats = await getDatabaseStats();
      setDbStats({
        ...stats,
        recording: isRecordingRef.current,
      });
    } catch (e) {
      console.error('Failed to get DB stats:', e);
    }
  }, []);

  useEffect(() => {
    refreshDbStats();
    const timer = setInterval(refreshDbStats, 5000);
    return () => clearInterval(timer);
  }, [refreshDbStats]);

  // Load historical records when entering history mode or changing range
  const loadHistoryRange = useCallback(async (range: HistoryRange) => {
    const now = Date.now();
    let startTime = 0;
    if (range === '10m') startTime = now - 10 * 60 * 1000;
    else if (range === '1h') startTime = now - 60 * 60 * 1000;
    else if (range === '6h') startTime = now - 6 * 60 * 60 * 1000;
    else if (range === '24h') startTime = now - 24 * 60 * 60 * 1000;
    else if (range === 'all') startTime = 0;

    try {
      const records = await getTelemetryRange(startTime, now, 300);
      setHistoryRecords(records);
      setScrubberIndex(records.length > 0 ? records.length - 1 : 0);
    } catch (e) {
      console.error('Failed to load history records:', e);
    }
  }, []);

  useEffect(() => {
    if (mode === 'history') {
      loadHistoryRange(historyRange);
    }
  }, [mode, historyRange, loadHistoryRange]);

  // WebSocket connection & live ingestion
  useEffect(() => {
    let ws: WebSocket;
    let reconnectTimeout: any;

    const connect = () => {
      setConnState('connecting');
      const url = `ws://${gatewayHost}/ws`;
      ws = new WebSocket(url);
      wsRef.current = ws;

      ws.onopen = () => {
        setConnState('connected');
      };

      ws.onclose = () => {
        setConnState('disconnected');
        reconnectTimeout = setTimeout(connect, 3000);
      };

      ws.onerror = () => {
        ws.close();
      };

      ws.onmessage = (event) => {
        try {
          const data: TelemetryData = JSON.parse(event.data);
          if (data.type === 'telemetry') {
            setTelemetry(data);

            // Persist to IndexedDB if recording is active
            if (isRecordingRef.current) {
              queueTelemetryRecord(data);
            }

            const now = new Date();
            const timeStr = `${now.getHours().toString().padStart(2, '0')}:${now
              .getMinutes()
              .toString()
              .padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`;

            setLiveHistory((prev) => {
              const newLabels = [...prev.labels, timeStr];
              const newAir = [...prev.airTemps, data.air_temp];
              const newLeaf = [...prev.leafTemps, data.leaf_temp];
              const newSoil = [...prev.soilRaws, data.soil_raw];
              const currentLoss =
                typeof data.ai_loss === 'number'
                  ? data.ai_loss
                  : prev.losses.length > 0
                  ? prev.losses[prev.losses.length - 1]
                  : 0.0245;
              const newLosses = [...prev.losses, currentLoss];

              if (newLabels.length > MAX_CHART_POINTS) {
                newLabels.shift();
                newAir.shift();
                newLeaf.shift();
                newSoil.shift();
                newLosses.shift();
              }

              return {
                labels: newLabels,
                airTemps: newAir,
                leafTemps: newLeaf,
                soilRaws: newSoil,
                losses: newLosses,
              };
            });
          }
        } catch (err) {
          console.error('Failed to parse telemetry message:', err);
        }
      };
    };

    connect();

    return () => {
      if (ws) ws.close();
      if (reconnectTimeout) clearTimeout(reconnectTimeout);
    };
  }, [gatewayHost]);

  // Selected telemetry to display across all widgets (Live or Scrubber position)
  const displayedTelemetry: TelemetryData = useMemo(() => {
    if (mode === 'history' && historyRecords.length > 0) {
      const idx = Math.min(Math.max(scrubberIndex, 0), historyRecords.length - 1);
      return historyRecords[idx];
    }
    return telemetry;
  }, [mode, historyRecords, scrubberIndex, telemetry]);

  // Scrubbed timestamp string
  const scrubbedTimeStr = useMemo(() => {
    if (mode === 'history' && historyRecords.length > 0) {
      const idx = Math.min(Math.max(scrubberIndex, 0), historyRecords.length - 1);
      const rec = historyRecords[idx];
      return new Date(rec.recordTime).toLocaleString('ja-JP', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
      });
    }
    return undefined;
  }, [mode, historyRecords, scrubberIndex]);

  // Chart datasets (Live rolling vs Historical range)
  const chartDatasets = useMemo(() => {
    if (mode === 'history') {
      const labels = historyRecords.map((r) => {
        const d = new Date(r.recordTime);
        return `${d.getHours().toString().padStart(2, '0')}:${d
          .getMinutes()
          .toString()
          .padStart(2, '0')}:${d.getSeconds().toString().padStart(2, '0')}`;
      });
      const airTemps = historyRecords.map((r) => r.air_temp);
      const leafTemps = historyRecords.map((r) => r.leaf_temp);
      const soilRaws = historyRecords.map((r) => r.soil_raw);
      const losses = historyRecords.map((r) => r.ai_loss ?? 0.025);

      return { labels, airTemps, leafTemps, soilRaws, losses };
    }
    return liveHistory;
  }, [mode, historyRecords, liveHistory]);

  // Handlers
  const handleToggleMode = (newMode: 'live' | 'history') => {
    setMode(newMode);
    if (newMode === 'history') {
      loadHistoryRange(historyRange);
    }
  };

  const handleChangeRange = (newRange: HistoryRange) => {
    setHistoryRange(newRange);
    loadHistoryRange(newRange);
  };

  const handleToggleRecording = () => {
    setIsRecording((prev) => !prev);
  };

  const handleClearHistory = async () => {
    if (window.confirm('ブラウザに保存されたすべての過去ログを消去しますか？')) {
      await clearAllRecords();
      setHistoryRecords([]);
      setScrubberIndex(0);
      refreshDbStats();
      alert('すべての過去ログを消去しました。');
    }
  };

  const handleExportAllCsv = async () => {
    try {
      const csv = await exportAllAsCsv();
      const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.setAttribute('href', url);
      link.setAttribute(
        'download',
        `plant_doctor_full_history_${new Date().toISOString().replace(/[:.]/g, '-')}.csv`
      );
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    } catch (e) {
      alert('CSVエクスポートに失敗しました: ' + e);
    }
  };

  const handleTriggerWatering = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send('water');
    } else {
      fetch(`http://${gatewayHost}/api/water`, { method: 'POST' }).catch((e) =>
        console.error(e)
      );
    }
  };

  const handleToggleDemo = (enable: boolean) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(enable ? 'demo_on' : 'demo_off');
    } else {
      fetch(`http://${gatewayHost}/api/demo?enable=${enable ? 1 : 0}`, {
        method: 'POST',
      }).catch((e) => console.error(e));
    }
  };

  const handleSyncTime = () => {
    const unix = Math.floor(Date.now() / 1000);
    fetch(`http://${gatewayHost}/api/time`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ unix }),
    })
      .then(() => alert('PCの現在時刻をDT-EBMLのRTCと同期しました。'))
      .catch((e) => alert('時刻同期エラー: ' + e));
  };

  const handleExportCsv = () => {
    const rows = [
      [
        'Time',
        'AirTemp(C)',
        'LeafTemp(C)',
        'Delta(C)',
        'SoilRaw',
        'Stress',
        'Status',
        'AiTrainCount',
        'AiLoss',
        'AiPhase',
        'AiAnomalyScore',
      ],
      ...chartDatasets.labels.map((lbl, i) => [
        lbl,
        chartDatasets.airTemps[i]?.toFixed(2),
        chartDatasets.leafTemps[i]?.toFixed(2),
        (chartDatasets.leafTemps[i] - chartDatasets.airTemps[i])?.toFixed(2),
        chartDatasets.soilRaws[i],
        displayedTelemetry.stress,
        displayedTelemetry.status,
        displayedTelemetry.ai_train_count ?? 0,
        chartDatasets.losses[i]?.toFixed(4) ?? '0.0000',
        displayedTelemetry.ai_phase ?? 0,
        displayedTelemetry.ai_anomaly_score ?? 0,
      ]),
    ];

    const csvContent =
      'data:text/csv;charset=utf-8,' + rows.map((e) => e.join(',')).join('\n');
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement('a');
    link.setAttribute('href', encodedUri);
    link.setAttribute(
      'download',
      `plant_telemetry_${new Date().toISOString().replace(/[:.]/g, '-')}.csv`
    );
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="max-w-7xl mx-auto px-4 py-8 space-y-6">
      <Header
        gatewayHost={gatewayHost}
        setGatewayHost={setGatewayHost}
        connState={connState}
        onSyncTime={handleSyncTime}
      />

      {/* History & Storage Control Toolbar */}
      <HistoryToolbar
        mode={mode}
        onToggleMode={handleToggleMode}
        historyRange={historyRange}
        onChangeRange={handleChangeRange}
        stats={dbStats}
        isRecording={isRecording}
        onToggleRecording={handleToggleRecording}
        onClearHistory={handleClearHistory}
        onExportHistoryCsv={handleExportAllCsv}
        scrubberIndex={scrubberIndex}
        scrubberMax={historyRecords.length > 0 ? historyRecords.length - 1 : 0}
        onScrub={setScrubberIndex}
        scrubbedTimeStr={scrubbedTimeStr}
      />

      {/* Solist-AI™ On-Device Learning Hero Station */}
      <SolistAiStatusCard
        phase={displayedTelemetry.ai_phase}
        trainCount={displayedTelemetry.ai_train_count}
        loss={displayedTelemetry.ai_loss}
        anomalyScore={displayedTelemetry.ai_anomaly_score}
        stress={displayedTelemetry.stress}
      />

      {/* Core Diagnostic & Environmental Status Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <StressMeter score={displayedTelemetry.stress} />
        <DiagnosisCard
          status={displayedTelemetry.status}
          soilTrend={displayedTelemetry.soil_trend}
        />
        <EnvironmentPanel
          airTemp={displayedTelemetry.air_temp}
          humidity={displayedTelemetry.humidity}
          leafTemp={displayedTelemetry.leaf_temp}
          leafAirDiff={displayedTelemetry.leaf_air_diff}
          soilRaw={displayedTelemetry.soil_raw}
          lux={displayedTelemetry.lux}
        />
        <WateringControl
          tankLiquid={displayedTelemetry.tank_liquid}
          pumpOn={displayedTelemetry.pump_on}
          demoMode={displayedTelemetry.demo_mode}
          onTriggerWatering={handleTriggerWatering}
          onToggleDemo={handleToggleDemo}
        />
      </div>

      {/* Solist-AI™ Deep Analysis Section: Loss Convergence & 8D Feature Radar */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <SolistAiLossChart
          timeLabels={chartDatasets.labels}
          lossHistory={chartDatasets.losses}
          isHistory={mode === 'history'}
        />
        <SolistAiFeatureRadar
          soilRaw={displayedTelemetry.soil_raw}
          leafTemp={displayedTelemetry.leaf_temp}
          airTemp={displayedTelemetry.air_temp}
          leafAirDiff={displayedTelemetry.leaf_air_diff}
          humidity={displayedTelemetry.humidity}
          lux={displayedTelemetry.lux}
          leafTempRate={displayedTelemetry.leaf_temp_rate}
          soilRate={displayedTelemetry.soil_rate}
        />
      </div>

      {/* Environmental Time Series Chart */}
      <TimeSeriesChart
        timeLabels={chartDatasets.labels}
        airTemps={chartDatasets.airTemps}
        leafTemps={chartDatasets.leafTemps}
        soilRaws={chartDatasets.soilRaws}
        onExportCsv={handleExportCsv}
        isHistory={mode === 'history'}
      />
    </div>
  );
}
