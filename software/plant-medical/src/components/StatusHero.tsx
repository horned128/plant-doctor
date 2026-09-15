import React from 'react';
import { PlantVisualizer } from './PlantVisualizer';
import { Droplets } from 'lucide-react';

interface StatusHeroProps {
  status: string;
  stress: number;
  soilTrend: string;
  soilRaw: number;
  tankLiquid: boolean;
  pumpOn: boolean;
  demoMode: boolean;
  onTriggerWatering: () => void;
  onToggleDemo: (enable: boolean) => void;
}

const STATUS_DESCRIPTIONS: Record<string, { title: string; desc: string; color: string; badgeBg: string }> = {
  HEALTHY: {
    title: '正常・健康 (Healthy)',
    desc: '平常状態を維持しています。葉温と気温の蒸散バランス、土壌水分ともに極めて安定しています。',
    color: 'text-emerald-400',
    badgeBg: 'bg-emerald-950/80 text-emerald-400 border-emerald-800',
  },
  HEAT_STRESS: {
    title: '熱ストレス (Heat Stress)',
    desc: '日照・気温上昇に対し、葉温の過度な上昇が検出されました。蒸散能力の限界が近づいています。',
    color: 'text-amber-400',
    badgeBg: 'bg-amber-950/80 text-amber-400 border-amber-800',
  },
  DRY_STRESS: {
    title: '乾燥ストレス (Dry Stress)',
    desc: '土壌水分の低下に伴い、気孔閉鎖による葉温上昇傾向が確認されました。給水が推奨されます。',
    color: 'text-orange-400',
    badgeBg: 'bg-orange-950/80 text-orange-400 border-orange-800',
  },
  WATERING: {
    title: '給水動作中 (Watering)',
    desc: 'ポンプが駆動中です。給水後の土壌水分の浸透と生体応答をリアルタイム監視しています。',
    color: 'text-sky-400',
    badgeBg: 'bg-sky-950/80 text-sky-400 border-sky-800',
  },
  WATERING_FAILED: {
    title: '給水失敗 (Failed)',
    desc: 'ポンプ動作後に土壌水分の回復が確認されませんでした。水タンク空やノズル外れを確認してください。',
    color: 'text-rose-400',
    badgeBg: 'bg-rose-950/80 text-rose-400 border-rose-800',
  },
  SOIL_SENSOR_ERROR: {
    title: 'センサ異常 (Sensor Error)',
    desc: '土壌水分センサの急激な離脱または異常値が検出されました。端子接続を確認してください。',
    color: 'text-purple-400',
    badgeBg: 'bg-purple-950/80 text-purple-400 border-purple-800',
  },
};

export const StatusHero: React.FC<StatusHeroProps> = ({
  status,
  stress,
  soilTrend,
  soilRaw,
  tankLiquid,
  pumpOn,
  demoMode,
  onTriggerWatering,
  onToggleDemo,
}) => {
  const info = STATUS_DESCRIPTIONS[status] || {
    title: status || 'UNKNOWN',
    desc: 'データを受信中またはAI診断待機中です。',
    color: 'text-slate-300',
    badgeBg: 'bg-slate-800 text-slate-400 border-slate-700',
  };

  // Dial arc math for 0 to 100
  const circumference = 251.2;
  const offset = circumference - (Math.min(Math.max(stress, 0), 100) / 100) * circumference;
  const strokeColor = stress >= 60 ? '#ef4444' : stress >= 30 ? '#f59e0b' : '#10b981';

  return (
    <div className="grid grid-cols-1 lg:grid-cols-12 gap-5 items-stretch">
      {/* Column 1: Plant Botanical Visualizer (5 Cols) */}
      <div className="lg:col-span-5 flex flex-col">
        <PlantVisualizer
          status={status}
          stress={stress}
          pumpOn={pumpOn}
          soilRaw={soilRaw}
        />
      </div>

      {/* Column 2: Diagnostic & Control Station (7 Cols) */}
      <div className="lg:col-span-7 flex flex-col justify-between gap-4">
        {/* Upper Card: Stress Meter & Diagnosis Summary */}
        <div className="bg-slate-900/90 border border-slate-800/80 rounded-2xl p-5 shadow-xl flex flex-col md:flex-row items-center gap-6">
          {/* Circular Stress Meter Dial */}
          <div className="relative w-36 h-36 flex-shrink-0 flex items-center justify-center">
            <svg className="w-full h-full transform -rotate-90" viewBox="0 0 100 100">
              <circle
                cx="50"
                cy="50"
                r="40"
                stroke="currentColor"
                strokeWidth="7"
                className="text-slate-800 fill-none"
              />
              <circle
                cx="50"
                cy="50"
                r="40"
                stroke={strokeColor}
                strokeWidth="7"
                strokeLinecap="round"
                strokeDasharray="251.2"
                strokeDashoffset={offset}
                className="transition-all duration-700 ease-out fill-none"
              />
            </svg>
            <div className="absolute flex flex-col items-center">
              <span className="text-4xl font-black text-white tracking-tight">{stress}</span>
              <span className="text-[10px] text-slate-400 uppercase font-semibold tracking-wider">
                Stress / 100
              </span>
            </div>
          </div>

          {/* Diagnosis Text Summary */}
          <div className="flex-1 text-center md:text-left">
            <div className="flex items-center justify-center md:justify-start gap-2 mb-1.5">
              <span className={`text-xs font-bold px-2.5 py-0.5 rounded-full border ${info.badgeBg}`}>
                {info.title}
              </span>
              <span className="text-xs font-mono text-slate-400 bg-slate-800/80 px-2 py-0.5 rounded border border-slate-700/60">
                土壌: {soilTrend || 'STABLE'}
              </span>
            </div>
            <h2 className={`text-xl font-bold ${info.color} mt-1 mb-1.5 tracking-tight`}>
              {status === 'HEALTHY' ? '平常・健康状態を維持中' : info.title}
            </h2>
            <p className="text-xs text-slate-300 leading-relaxed line-clamp-2">
              {info.desc}
            </p>
          </div>
        </div>

        {/* Lower Card: Watering Control & Safe Interlock Station */}
        <div className="bg-slate-900/90 border border-slate-800/80 rounded-2xl p-5 shadow-xl flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="flex flex-wrap items-center gap-2.5 w-full sm:w-auto">
            {/* Tank status badge */}
            <div
              className={`flex items-center space-x-1.5 px-3 py-1.5 rounded-xl text-xs font-bold border ${
                tankLiquid
                  ? 'bg-emerald-950/60 text-emerald-400 border-emerald-800'
                  : 'bg-rose-950/60 text-rose-400 border-rose-800 animate-pulse'
              }`}
            >
              <span className={`w-2 h-2 rounded-full ${tankLiquid ? 'bg-emerald-400' : 'bg-rose-500'}`} />
              <span>{tankLiquid ? '水タンク満水 (OK)' : 'タンク空 (給水拒否)'}</span>
            </div>

            {/* Pump status badge */}
            <div
              className={`flex items-center space-x-1.5 px-3 py-1.5 rounded-xl text-xs font-bold border ${
                pumpOn
                  ? 'bg-sky-950/60 text-sky-400 border-sky-800 animate-pulse'
                  : 'bg-slate-800/60 text-slate-400 border-slate-700'
              }`}
            >
              <span className={`w-2 h-2 rounded-full ${pumpOn ? 'bg-sky-400 animate-ping' : 'bg-slate-500'}`} />
              <span>{pumpOn ? 'ポンプ稼働中 (ON)' : 'ポンプ待機 (Standby)'}</span>
            </div>

            {/* Demo mode toggle */}
            <div className="flex items-center space-x-2 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800 text-xs">
              <span className="text-slate-400 text-[11px]">デモ:</span>
              <input
                type="checkbox"
                checked={demoMode}
                onChange={(e) => onToggleDemo(e.target.checked)}
                className="accent-indigo-500 cursor-pointer w-3.5 h-3.5"
              />
              <span className={`text-[11px] font-medium ${demoMode ? 'text-indigo-400 font-bold' : 'text-slate-500'}`}>
                {demoMode ? 'ON' : 'OFF'}
              </span>
            </div>
          </div>

          {/* Quick Action Button */}
          <button
            onClick={onTriggerWatering}
            disabled={!tankLiquid || pumpOn}
            className={`w-full sm:w-auto px-5 py-2.5 rounded-xl font-bold text-xs shadow-lg transition-all flex items-center justify-center space-x-2 flex-shrink-0 ${
              tankLiquid && !pumpOn
                ? 'bg-sky-600 hover:bg-sky-500 text-white shadow-sky-600/30 active:scale-95 cursor-pointer'
                : 'bg-slate-800 text-slate-500 border border-slate-700 cursor-not-allowed'
            }`}
          >
            <Droplets className="w-4 h-4" />
            <span>手動給水テスト (SW4)</span>
          </button>
        </div>
      </div>
    </div>
  );
};
