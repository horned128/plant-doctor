import { useState, useEffect, useRef, useCallback } from 'react';
import {
  TelemetryData,
  HistorySampleRecord,
  ConnectionState,
  TimeResolution,
  PlantProfile,
} from './types';
import { CockpitHeader, CockpitTab } from './components/CockpitHeader';
import { DigitalPlantTwin } from './components/DigitalPlantTwin';
import { BioTelemetryPanel } from './components/BioTelemetryPanel';
import { AiDiagnosisPanel } from './components/AiDiagnosisPanel';
import { RootZoneWatering } from './components/RootZoneWatering';
import { HistoryAnalyticsView } from './components/HistoryAnalyticsView';
import { SolistAiStatusCard } from './components/SolistAiStatusCard';
import { SolistAiLossChart } from './components/SolistAiLossChart';
import { SolistAiFeatureRadar } from './components/SolistAiFeatureRadar';
import { SystemInterlockModal } from './components/SystemInterlockModal';
import {
  loadPlantProfiles,
  savePlantProfiles,
  loadActiveProfileId,
  saveActiveProfileId,
  updateProfileWithPersonalLearning,
} from './services/plantProfileService';
import {
  fetchServerHistory,
  clearServerHistory,
  saveServerHistorySample,
  downloadServerParquetFile,
  downloadServerCsvFile,
} from './services/historyApi';
import { exportHistoryToParquet, exportHistoryToCsv } from './services/parquetExporter';
import { Cpu } from 'lucide-react';

const MAX_CHART_POINTS = 60; // 60 seconds for live view

export default function App() {
  const [gatewayHost, setGatewayHost] = useState('plant-doctor.local');
  const [connState, setConnState] = useState<ConnectionState>('connecting');
  
  // デフォルトタブ: 植物を主役にした 1画面完結型診察室 (cockpit)
  const [activeTab, setActiveTab] = useState<CockpitTab>('cockpit');
  const [timeResolution, setTimeResolution] = useState<TimeResolution>('24h');

  // 植物プロファイル管理ステート
  const [profiles, setProfiles] = useState<PlantProfile[]>(() => loadPlantProfiles());
  const [activeProfileId, setActiveProfileId] = useState<string>(() => loadActiveProfileId());
  const [isTrainingPersonal, setIsTrainingPersonal] = useState(false);

  const activeProfile =
    profiles.find((p) => p.id === activeProfileId) || profiles[0];

  // Live telemetry state (健康時のストレススコアは 0〜5 のフレキシブルな値)
  const [telemetry, setTelemetry] = useState<TelemetryData>({
    type: 'telemetry',
    timestamp: Math.floor(Date.now() / 1000),
    seq: 1,
    stress: 2, // 0〜100フルレンジ活用、完全正常時は極小値
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
    ai_loss: 0.0215,
    ai_phase: 2,
    ai_anomaly_score: 4,
    leaf_temp_rate: -0.2,
    soil_rate: -12.0,
  });

  // Short live rolling history (60 sec) for loss charts
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
  const [isLoadingHistory, setIsLoadingHistory] = useState(false);

  const wsRef = useRef<WebSocket | null>(null);
  const lastSampleTimeRef = useRef<number>(0);

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
                  : 0.0215;
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

            // 10分サンプリングログをサーバPC側に蓄積保存
            if (
              data.valid &&
              (data.timestamp - lastSampleTimeRef.current >= 600 ||
                lastSampleTimeRef.current === 0)
            ) {
              lastSampleTimeRef.current = data.timestamp;
              const sampleRecord: HistorySampleRecord = {
                timestamp: data.timestamp,
                seq: data.seq,
                stress: data.stress,
                status: data.status,
                soil_trend: data.soil_trend,
                air_temp: data.air_temp,
                humidity: data.humidity,
                leaf_temp: data.leaf_temp,
                leaf_air_diff: data.leaf_air_diff,
                soil_raw: data.soil_raw,
                lux: data.lux,
                tank_liquid: data.tank_liquid,
                pump_on: data.pump_on,
                demo_mode: data.demo_mode,
                ai_train_count: data.ai_train_count,
                ai_loss: data.ai_loss,
                ai_phase: data.ai_phase,
                ai_anomaly_score: data.ai_anomaly_score,
                leaf_temp_rate: data.leaf_temp_rate,
                soil_rate: data.soil_rate,
              };
              saveServerHistorySample(sampleRecord);
              setHistoryRecords((prev) => [...prev, sampleRecord]);
            }
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

  // Handlers for Plant Profile
  const handleSelectProfile = (id: string) => {
    setActiveProfileId(id);
    saveActiveProfileId(id);
  };

  const handleUpdateProfileName = (id: string, newName: string) => {
    const updated = profiles.map((p) => (p.id === id ? { ...p, name: newName } : p));
    setProfiles(updated);
    savePlantProfiles(updated);
  };

  const handleTrainPersonalModel = (id: string) => {
    setIsTrainingPersonal(true);
    setTimeout(() => {
      const clamp = (val: number, min: number, max: number) =>
        Math.min(Math.max(val, min), max);

      const curFeatures = [
        clamp(Math.round(((2400 - telemetry.soil_raw) / (2400 - 1200)) * 100), 0, 100),
        clamp(Math.round(((telemetry.leaf_temp - 15) / 20) * 100), 0, 100),
        clamp(Math.round(((telemetry.air_temp - 15) / 20) * 100), 0, 100),
        clamp(Math.round(((telemetry.leaf_air_diff - -4.0) / 6.0) * 100), 0, 100),
        clamp(Math.round(((telemetry.humidity - 20) / 80) * 100), 0, 100),
        clamp(Math.round(45 + ((telemetry.lux - 200) / 650) * 10), 45, 55),
        clamp(Math.round(((telemetry.leaf_temp_rate ?? 0 - -5.0) / 10.0) * 100), 0, 100),
        clamp(Math.round(((telemetry.soil_rate ?? 0 - -200) / 400) * 100), 0, 100),
      ];

      const target = profiles.find((p) => p.id === id);
      if (target) {
        const trained = updateProfileWithPersonalLearning(
          target,
          curFeatures,
          telemetry.ai_loss ?? 0.0215
        );
        const updated = profiles.map((p) => (p.id === id ? trained : p));
        setProfiles(updated);
        savePlantProfiles(updated);
        alert(
          `【${target.name}】のパーソナル生体基準モデルを更新・保存しました！\nこの植物固有の環境特徴量（8次元）が反映されます。`
        );
      }
      setIsTrainingPersonal(false);
    }, 800);
  };

  // Handlers for Device Control
  const handleTriggerWatering = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send('water');
    } else {
      fetch(`http://${gatewayHost}/api/water`, { method: 'POST' }).catch((e) =>
        console.error(e)
      );
    }
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
        stress: Math.max(0, prev.stress - 15),
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
      stress: enable ? 65 : 2,
    }));
  };

  const handleSyncTime = () => {
    const unix = Math.floor(Date.now() / 1000);
    fetch(`http://${gatewayHost}/api/time`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ unix }),
    })
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then(() => {
        alert('PCの現在時刻をDT-EBMLのRTCと同期しました。');
        loadHistory();
      })
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
    try {
      downloadServerCsvFile();
    } catch {
      exportHistoryToCsv(historyRecords);
    }
  };

  const handleExportParquet = () => {
    try {
      downloadServerParquetFile();
    } catch {
      exportHistoryToParquet(historyRecords);
    }
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col bg-cockpit-grid">
      {/* 1. Sleek Cockpit Header (4 Menus) */}
      <CockpitHeader
        activeTab={activeTab}
        onChangeTab={setActiveTab}
        activeProfile={activeProfile}
        profiles={profiles}
        onSelectProfile={handleSelectProfile}
        onUpdateProfileName={handleUpdateProfileName}
        onTrainPersonalModel={handleTrainPersonalModel}
        isTrainingPersonal={isTrainingPersonal}
        connState={connState}
      />

      {/* 2. Main Content Container */}
      <main className="flex-1 w-full max-w-[1600px] mx-auto p-3 sm:p-4 lg:p-6 flex flex-col justify-start">
        {/* ============================================================ */}
        {/* MENU 1: MEDICAL COCKPIT (NOW) - 植物主役の 1画面完結型診察室 */}
        {/* ============================================================ */}
        {activeTab === 'cockpit' && (
          <div className="w-full flex flex-col gap-4 flex-1 justify-between">
            {/* Upper Cockpit: 3-Column Biological Stage */}
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-4 flex-1 items-stretch">
              {/* Left Wing (3.5 Cols): 生体 & 環境テレメトリ (葉温, ΔT, 気温, 湿度, 照度) */}
              <div className="lg:col-span-3 xl:col-span-3 flex flex-col">
                <BioTelemetryPanel
                  airTemp={telemetry.air_temp}
                  humidity={telemetry.humidity}
                  leafTemp={telemetry.leaf_temp}
                  leafAirDiff={telemetry.leaf_air_diff}
                  lux={telemetry.lux}
                  leafTempRate={telemetry.leaf_temp_rate}
                  soilRate={telemetry.soil_rate}
                />
              </div>

              {/* Center Stage (5.5 Cols): 植物生体デジタルツイン (生命の呼吸 & 状態可視化) */}
              <div className="lg:col-span-5 xl:col-span-5 flex flex-col">
                <DigitalPlantTwin
                  status={telemetry.status}
                  stress={telemetry.stress}
                  pumpOn={telemetry.pump_on}
                  soilRaw={telemetry.soil_raw}
                  leafTemp={telemetry.leaf_temp}
                  leafAirDiff={telemetry.leaf_air_diff}
                  lux={telemetry.lux}
                  interactive={true}
                />
              </div>

              {/* Right Wing (3 Cols): Solist-AI™ 臨床診断 & Why? 主要因分析 */}
              <div className="lg:col-span-4 xl:col-span-4 flex flex-col">
                <AiDiagnosisPanel
                  status={telemetry.status}
                  stress={telemetry.stress}
                  soilTrend={telemetry.soil_trend}
                  soilRaw={telemetry.soil_raw}
                  leafAirDiff={telemetry.leaf_air_diff}
                  airTemp={telemetry.air_temp}
                  lux={telemetry.lux}
                  aiLoss={telemetry.ai_loss}
                  aiPhase={telemetry.ai_phase}
                  aiTrainCount={telemetry.ai_train_count}
                  onNavigateToAiLab={() => setActiveTab('solist')}
                />
              </div>
            </div>

            {/* Lower Cockpit: 根圏土壌水分 & 給水安全インターロック ステーション */}
            <div className="w-full mt-1">
              <RootZoneWatering
                soilRaw={telemetry.soil_raw}
                soilTrend={telemetry.soil_trend}
                tankLiquid={telemetry.tank_liquid}
                pumpOn={telemetry.pump_on}
                demoMode={telemetry.demo_mode}
                onTriggerWatering={handleTriggerWatering}
                onToggleDemo={handleToggleDemo}
              />
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* MENU 2: HISTORY & ANALYTICS (UNIFIED: 時系列推移 ＆ 履歴ログ) */}
        {/* ============================================================ */}
        {activeTab === 'history' && (
          <div className="w-full">
            <HistoryAnalyticsView
              records={historyRecords}
              resolution={timeResolution}
              onChangeResolution={setTimeResolution}
              onRefresh={loadHistory}
              onClear={handleClearHistory}
              onExportCsv={handleExportCsv}
              onExportParquet={handleExportParquet}
              isLoading={isLoadingHistory}
            />
          </div>
        )}

        {/* ============================================================ */}
        {/* MENU 3: SOLIST-AI™ LAB (8次元特徴空間レーダー & ODL学習収束) */}
        {/* ============================================================ */}
        {activeTab === 'solist' && (
          <div className="w-full space-y-6">
            <SolistAiStatusCard
              phase={telemetry.ai_phase}
              trainCount={telemetry.ai_train_count}
              loss={telemetry.ai_loss}
              anomalyScore={telemetry.ai_anomaly_score}
              stress={telemetry.stress}
            />

            {/* 8次元生体特徴空間レーダー & 再構成誤差推移 */}
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              <SolistAiFeatureRadar
                soilRaw={telemetry.soil_raw}
                leafTemp={telemetry.leaf_temp}
                airTemp={telemetry.air_temp}
                leafAirDiff={telemetry.leaf_air_diff}
                humidity={telemetry.humidity}
                lux={telemetry.lux}
                leafTempRate={telemetry.leaf_temp_rate}
                soilRate={telemetry.soil_rate}
                activeProfile={activeProfile}
              />

              <SolistAiLossChart
                timeLabels={liveHistory.labels}
                lossHistory={liveHistory.losses}
                isHistory={false}
                phase={telemetry.ai_phase}
                trainCount={telemetry.ai_train_count}
                anomalyScore={telemetry.ai_anomaly_score}
                currentLoss={telemetry.ai_loss}
              />
            </div>

            {/* 原理解説 */}
            <div className="p-5 rounded-3xl bg-slate-900/80 border border-slate-800 text-xs text-slate-300 space-y-2">
              <h4 className="font-bold text-white flex items-center gap-2 text-sm">
                <Cpu className="w-4 h-4 text-teal-400" />
                <span>オンデバイス・オートエンコーダ（ODL / OSUAD）の動作原理</span>
              </h4>
              <p className="text-slate-400 leading-relaxed">
                ROHM ML63Q2557 内蔵の Solist-AI™ ハードウェアアクセラレータは、8 次元の植物生体・環境特徴量ベクトルを入力とし、64 中間層を介して自己再構成を行います。
                平常稼働時に逐次再帰最小二乗法（RLS）を用いてモデル重みをリアルタイム学習し、再構成損失（MSE）の上昇から未知の環境ストレスや生体異常を予兆検知します。
              </p>
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* MENU 4: SYSTEM & INTERLOCK (ハードウェア・通信・RTC同期) */}
        {/* ============================================================ */}
        {activeTab === 'system' && (
          <div className="w-full">
            <SystemInterlockModal
              gatewayHost={gatewayHost}
              setGatewayHost={setGatewayHost}
              connState={connState}
              onSyncTime={handleSyncTime}
              tankLiquid={telemetry.tank_liquid}
              pumpOn={telemetry.pump_on}
              seq={telemetry.seq}
              timestamp={telemetry.timestamp}
            />
          </div>
        )}
      </main>
    </div>
  );
}
