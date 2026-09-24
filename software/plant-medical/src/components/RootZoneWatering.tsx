import React from 'react';
import { Layers, Droplets, AlertTriangle } from 'lucide-react';

interface RootZoneWateringProps {
  soilRaw: number;
  soilTrend: string;
  tankLiquid: boolean;
  pumpOn: boolean;
  demoMode: boolean;
  onTriggerWatering: () => void;
  onToggleDemo: (enable: boolean) => void;
}

export const RootZoneWatering: React.FC<RootZoneWateringProps> = ({
  soilRaw,
  soilTrend,
  tankLiquid,
  pumpOn,
  demoMode,
  onTriggerWatering,
  onToggleDemo,
}) => {
  // Soil Raw normalization: ~1200 wet (100%), ~2400 dry (0%)
  const soilMoisturePercent = Math.min(
    100,
    Math.max(0, Math.round(((2400 - soilRaw) / (2400 - 1200)) * 100))
  );

  const isSoilOptimal = soilRaw < 2100;

  return (
    <div className="w-full bg-slate-950/40 border border-slate-800/60 rounded-3xl p-4 backdrop-blur-md shadow-xl flex flex-col md:flex-row items-center justify-between gap-4">
      {/* Left: Root Zone Soil Moisture */}
      <div className="flex-1 flex flex-col sm:flex-row items-start sm:items-center gap-4 w-full md:w-auto">
        <div className="flex items-center space-x-3">
          <div className="p-2.5 rounded-2xl bg-amber-500/10 border border-amber-500/30 text-amber-400">
            <Layers className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="text-xs font-bold text-slate-300">根圏土壌水分</span>
              <span className="text-[9px] font-mono text-slate-500 bg-slate-900 px-1.5 py-0.5 rounded border border-slate-800">
                SEN0193
              </span>
            </div>
            <div className="flex items-baseline gap-1.5">
              <span className="text-xl font-black text-white font-mono">{soilRaw}</span>
              <span className="text-[10px] text-slate-500 font-mono">Raw</span>
              <span className="text-xs font-bold text-slate-400 ml-1">
                ({soilMoisturePercent}%)
              </span>
            </div>
          </div>
        </div>

        {/* Moisture bar & Trend */}
        <div className="flex-1 max-w-xs w-full space-y-1">
          <div className="flex items-center justify-between text-[10px] font-mono text-slate-400">
            <span>
              {isSoilOptimal ? (
                <span className="text-emerald-400 font-semibold">潤沢・安定</span>
              ) : (
                <span className="text-amber-400 font-semibold">水分低下 (給水推奨)</span>
              )}
            </span>
            <span className="bg-slate-900 px-2 py-0.5 rounded text-sky-400 font-semibold border border-slate-800">
              浸透: {soilTrend || 'STABLE'}
            </span>
          </div>
          <div className="w-full bg-slate-900 h-2 rounded-full overflow-hidden border border-slate-800">
            <div
              className={`h-full transition-all duration-700 rounded-full ${
                soilMoisturePercent > 40
                  ? 'bg-gradient-to-r from-teal-500 to-sky-400'
                  : 'bg-gradient-to-r from-amber-500 to-rose-500'
              }`}
              style={{ width: `${Math.max(5, soilMoisturePercent)}%` }}
            />
          </div>
        </div>
      </div>

      {/* Middle: Tank & Interlock Badges */}
      <div className="flex items-center gap-2.5">
        {tankLiquid ? (
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-emerald-950/60 border border-emerald-700/50 text-emerald-400 text-xs font-bold shadow-sm">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span>給水タンク満水 (OK)</span>
          </div>
        ) : (
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-rose-950/80 border border-rose-600 text-rose-400 text-xs font-bold animate-pulse shadow-md">
            <AlertTriangle className="w-3.5 h-3.5" />
            <span>タンク空 (給水インターロック作動)</span>
          </div>
        )}

        {pumpOn ? (
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-sky-950/80 border border-sky-600 text-sky-400 text-xs font-bold animate-pulse shadow-md">
            <Droplets className="w-3.5 h-3.5 animate-bounce" />
            <span>ポンプ駆動中</span>
          </div>
        ) : (
          <div className="hidden sm:flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-900/80 border border-slate-800 text-slate-400 text-xs font-medium">
            <span>待機中 (Standby)</span>
          </div>
        )}
      </div>

      {/* Right: Actions (Water Button & Demo Toggle) */}
      <div className="flex items-center gap-3 w-full md:w-auto justify-end">
        {/* Demo Switch */}
        <div className="flex items-center gap-2 bg-slate-900/80 px-3 py-1.5 rounded-xl border border-slate-800 text-xs">
          <span className="text-slate-400 text-[11px]">デモ乾燥再現:</span>
          <label className="relative inline-flex items-center cursor-pointer">
            <input
              type="checkbox"
              checked={demoMode}
              onChange={(e) => onToggleDemo(e.target.checked)}
              className="sr-only peer"
            />
            <div className="w-8 h-4 bg-slate-800 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:rounded-full after:h-3 after:w-3 after:transition-all peer-checked:bg-amber-600" />
          </label>
        </div>

        {/* Trigger Water Button */}
        <button
          onClick={onTriggerWatering}
          disabled={!tankLiquid || pumpOn}
          className={`flex items-center space-x-1.5 px-4 py-2 rounded-xl text-xs font-bold transition shadow-lg cursor-pointer ${
            tankLiquid && !pumpOn
              ? 'bg-gradient-to-r from-sky-600 to-teal-500 hover:from-sky-500 hover:to-teal-400 text-white shadow-sky-600/30 active:scale-95'
              : 'bg-slate-800 text-slate-500 cursor-not-allowed border border-slate-700'
          }`}
          title="DT-EBMLの給水ポンプを手動駆動 (最大2.0秒)"
        >
          <Droplets className="w-4 h-4" />
          <span>手動給水テスト</span>
        </button>
      </div>
    </div>
  );
};
