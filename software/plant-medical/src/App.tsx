import { useState, useEffect, useRef, useCallback } from 'react';
import {
  TelemetryData,
  HistorySampleRecord,
  ConnectionState,
  HistoryRange,
  ActiveTab,
} from './types';
import { Header } from './components/Header';
import { StatusHero } from './components/StatusHero';
import { MetricGrid } from './components/MetricGrid';
import { HistoryLogTable } from './components/HistoryLogTable';
import { TimeSeriesChart } from './components/TimeSeriesChart';
import { SolistAiStatusCard } from './components/SolistAiStatusCard';
import { SolistAiLossChart } from './components/SolistAiLossChart';
import { SolistAiFeatureRadar } from './components/SolistAiFeatureRadar';
import {
  fetchServerHistory,
  clearServerHistory,
} from './services/historyApi';
import { Server, LineChart, Cpu } from 'lucide-react';

const MAX_CHART_POINTS = 60; // 60 seconds for live view

export default function App() {
  const [gatewayHost, setGatewayHost] = useState('plant-doctor.local');
  const [connState, setConnState] = useState<ConnectionState>('connecting');
  const [activeTab, setActiveTab] = useState<ActiveTab>('logs');

  // Live telemetry state
  const [telemetry, setTelemetry] = useState<TelemetryData>({
    type: 'telemetry',
    timestamp: Math.floor(Date.now() / 1000),
    seq: 1,
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

  // Short live rolling history (60 sec) for charts
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

  // Server-side 10-minute downsampled history state
  const [historyRecords, setHistoryRecords] = useState<HistorySampleRecord[]>([]);
  const [historyRange, setHistoryRange] = useState<HistoryRange>('24h');
  const [isLoadingHistory, setIsLoadingHistory] = useState(false);

  const wsRef = useRef<WebSocket | null>(null);

  // Load server-side 10-minute history
  const loadHistory = useCallback(async () => {
    setIsLoadingHistory(true);
    try {
      const records = await fetchServerHistory(gatewayHost);
      setHistoryRecords(records);
    } catch (e) {
      console.error('Failed to load server history:', e);
    } finally {
      setIsLoadingHistory(false);
    }
  }, [gatewayHost]);

  // Initial history load and periodic polling (every 60 seconds)
  useEffect(() => {
    loadHistory();
    const timer = setInterval(loadHistory, 60000);
    return () => clearInterval(timer);
  }, [loadHistory]);

  // WebSocket connection & live streaming
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

  // Handlers
  const handleTriggerWatering = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send('water');
    } else {
      fetch(`http://${gatewayHost}/api/water`, { method: 'POST' }).catch((e) =>
        console.error(e)
      );
    }
    // Optimistic UI update
    setTelemetry((prev) => ({
      ...prev,
      pump_on: true,
      status: 'WATERING',
    }));
    setTimeout(() => {
      setTelemetry((prev) => ({
        ...prev,
        pump_on: false,
        status: 'HEALTHY',
        stress: Math.max(10, prev.stress - 15),
      }));
      loadHistory();
    }, 2500);
  };

  const handleToggleDemo = (enable: boolean) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(enable ? 'demo_on' : 'demo_off');
    } else {
      fetch(`http://${gatewayHost}/api/demo?enable=${enable ? 1 : 0}`, {
        method: 'POST',
      }).catch((e) => console.error(e));
    }
    setTelemetry((prev) => ({
      ...prev,
      demo_mode: enable,
      status: enable ? 'DRY_STRESS' : 'HEALTHY',
      stress: enable ? 65 : 18,
    }));
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

  const handleClearHistory = async () => {
    if (window.confirm('サーバに保存された10分サンプリング履歴ログを消去しますか？')) {
      await clearServerHistory(gatewayHost);
      setHistoryRecords([]);
      alert('履歴ログを消去しました。');
    }
  };

  const handleExportCsv = () => {
    const rows = [
      ['Time', 'AirTemp(C)', 'LeafTemp(C)', 'Delta(C)', 'SoilRaw', 'Stress', 'Status'],
      ...liveHistory.labels.map((lbl, i) => [
        lbl,
        liveHistory.airTemps[i]?.toFixed(2),
        liveHistory.leafTemps[i]?.toFixed(2),
        (liveHistory.leafTemps[i] - liveHistory.airTemps[i])?.toFixed(2),
        liveHistory.soilRaws[i],
        telemetry.stress,
        telemetry.status,
      ]),
    ];

    const csvContent =
      'data:text/csv;charset=utf-8,' + rows.map((e) => e.join(',')).join('\n');
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement('a');
    link.setAttribute('href', encodedUri);
    link.setAttribute(
      'download',
      `plant_telemetry_live_${new Date().toISOString().replace(/[:.]/g, '-')}.csv`
    );
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="max-w-6xl mx-auto px-4 py-6 space-y-6">
      {/* 1. Header (Minimalist & Crisp) */}
      <Header
        gatewayHost={gatewayHost}
        setGatewayHost={setGatewayHost}
        connState={connState}
        onSyncTime={handleSyncTime}
      />

      {/* 2. Hero Section: Plant Visualizer + Stress Meter & Actions */}
      <StatusHero
        status={telemetry.status}
        stress={telemetry.stress}
        soilTrend={telemetry.soil_trend}
        soilRaw={telemetry.soil_raw}
        tankLiquid={telemetry.tank_liquid}
        pumpOn={telemetry.pump_on}
        demoMode={telemetry.demo_mode}
        onTriggerWatering={handleTriggerWatering}
        onToggleDemo={handleToggleDemo}
      />

      {/* 3. Environmental & Biological Metric Grid */}
      <MetricGrid
        airTemp={telemetry.air_temp}
        humidity={telemetry.humidity}
        leafTemp={telemetry.leaf_temp}
        leafAirDiff={telemetry.leaf_air_diff}
        soilRaw={telemetry.soil_raw}
        lux={telemetry.lux}
      />

      {/* 4. Tab Navigation (Declutters UI into intuitive workspaces) */}
      <div className="flex items-center justify-between border-b border-slate-800 pb-3 pt-2">
        <div className="flex items-center space-x-2 bg-slate-900/90 p-1.5 rounded-2xl border border-slate-800">
          <button
            onClick={() => setActiveTab('logs')}
            className={`flex items-center space-x-2 px-4 py-2 rounded-xl text-xs font-bold transition-all cursor-pointer ${
              activeTab === 'logs'
                ? 'bg-indigo-600 text-white shadow-lg shadow-indigo-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Server className="w-4 h-4" />
            <span>10分サンプリング履歴ログ</span>
          </button>

          <button
            onClick={() => setActiveTab('charts')}
            className={`flex items-center space-x-2 px-4 py-2 rounded-xl text-xs font-bold transition-all cursor-pointer ${
              activeTab === 'charts'
                ? 'bg-emerald-600 text-white shadow-lg shadow-emerald-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <LineChart className="w-4 h-4" />
            <span>環境時系列推移</span>
          </button>

          <button
            onClick={() => setActiveTab('ai')}
            className={`flex items-center space-x-2 px-4 py-2 rounded-xl text-xs font-bold transition-all cursor-pointer ${
              activeTab === 'ai'
                ? 'bg-teal-600 text-white shadow-lg shadow-teal-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Cpu className="w-4 h-4" />
            <span>Solist-AI™ 分析</span>
          </button>
        </div>

        <span className="text-xs text-slate-500 hidden sm:inline">
          ROHM ML63Q2557 生体モニタリングステーション
        </span>
      </div>

      {/* 5. Tab Content Panes */}
      <div>
        {/* Tab 1: Server 10-Minute Downsampled Logs */}
        {activeTab === 'logs' && (
          <HistoryLogTable
            records={historyRecords}
            range={historyRange}
            onChangeRange={setHistoryRange}
            onRefresh={loadHistory}
            onClear={handleClearHistory}
            isLoading={isLoadingHistory}
          />
        )}

        {/* Tab 2: Environmental Time Series Charts */}
        {activeTab === 'charts' && (
          <TimeSeriesChart
            timeLabels={liveHistory.labels}
            airTemps={liveHistory.airTemps}
            leafTemps={liveHistory.leafTemps}
            soilRaws={liveHistory.soilRaws}
            onExportCsv={handleExportCsv}
            isHistory={false}
          />
        )}

        {/* Tab 3: Solist-AI™ On-Device Learning & Feature Analysis */}
        {activeTab === 'ai' && (
          <div className="space-y-6">
            <SolistAiStatusCard
              phase={telemetry.ai_phase}
              trainCount={telemetry.ai_train_count}
              loss={telemetry.ai_loss}
              anomalyScore={telemetry.ai_anomaly_score}
              stress={telemetry.stress}
            />

            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              <SolistAiLossChart
                timeLabels={liveHistory.labels}
                lossHistory={liveHistory.losses}
                isHistory={false}
              />
              <SolistAiFeatureRadar
                soilRaw={telemetry.soil_raw}
                leafTemp={telemetry.leaf_temp}
                airTemp={telemetry.air_temp}
                leafAirDiff={telemetry.leaf_air_diff}
                humidity={telemetry.humidity}
                lux={telemetry.lux}
                leafTempRate={telemetry.leaf_temp_rate}
                soilRate={telemetry.soil_rate}
              />
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
