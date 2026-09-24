import React from 'react';
import { Thermometer, Droplets, Leaf, Activity, SunMedium, ArrowUpRight, ArrowDownRight } from 'lucide-react';

interface BioTelemetryPanelProps {
  airTemp: number;
  humidity: number;
  leafTemp: number;
  leafAirDiff: number;
  lux: number;
  leafTempRate?: number;
  soilRate?: number;
}

export const BioTelemetryPanel: React.FC<BioTelemetryPanelProps> = ({
  airTemp,
  humidity,
  leafTemp,
  leafAirDiff,
  lux,
  leafTempRate = 0,
  soilRate = 0,
}) => {
  const diffSign = leafAirDiff >= 0 ? '+' : '';
  const isHealthyTranspiration = leafAirDiff < 0;

  // Lux Zone assessment
  let luxZoneLabel = '好適光合成域';
  let luxBarPercent = Math.min(100, Math.max(0, (lux / 6000) * 100));
  if (lux < 100) {
    luxZoneLabel = '暗期・弱光域';
  } else if (lux > 5000) {
    luxZoneLabel = '強光・直射注意';
  }

  // Transpiration Efficiency Percentage (-3.0C is ~100%, 0.0C is 0%)
  const transpirationScore = Math.min(100, Math.max(0, Math.round(((-leafAirDiff) / 2.5) * 100)));

  return (
    <div className="h-full flex flex-col justify-between bg-slate-950/40 border border-slate-800/60 rounded-3xl p-5 backdrop-blur-md relative overflow-hidden shadow-2xl">
      {/* Background Accent Subtle Glow */}
      <div className="absolute -top-16 -left-16 w-44 h-44 bg-teal-500/10 rounded-full blur-3xl pointer-events-none" />

      {/* Header */}
      <div className="flex items-center justify-between border-b border-slate-800/80 pb-3">
        <div className="flex items-center space-x-2">
          <div className="w-2 h-2 rounded-full bg-teal-400 animate-pulse" />
          <h3 className="text-xs font-bold uppercase tracking-widest text-slate-300">
            生体 ＆ 環境テレメトリ
          </h3>
        </div>
        <span className="text-[10px] text-teal-400/80 bg-teal-950/60 px-2 py-0.5 rounded border border-teal-800/50">
          リアルタイム生体値
        </span>
      </div>

      {/* Section 1: 最重要・蒸散冷却（Transpiration & Leaf Dynamics） */}
      <div className="my-3 p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 via-slate-900/60 to-slate-950/80 border border-slate-800/90 shadow-inner">
        <div className="flex items-center justify-between mb-2">
          <div className="flex items-center gap-1.5 text-xs font-semibold text-teal-300">
            <Activity className="w-4 h-4 text-teal-400" />
            <span>蒸散冷却指標 (&Delta;T)</span>
          </div>
          <span className="text-[10px] text-slate-400">蒸散活動</span>
        </div>

        <div className="flex items-baseline justify-between">
          <div className="flex items-baseline gap-1">
            <span
              className={`text-3xl sm:text-4xl font-black tracking-tight font-mono ${
                isHealthyTranspiration ? 'text-emerald-400' : 'text-amber-400'
              }`}
            >
              {diffSign}{leafAirDiff !== undefined ? leafAirDiff.toFixed(2) : '--'}
            </span>
            <span className="text-sm font-semibold text-slate-400">&deg;C</span>
          </div>

          <div className="text-right">
            <div className="text-[11px] font-bold text-slate-300">
              {isHealthyTranspiration ? (
                <span className="text-emerald-400 flex items-center gap-1 justify-end">
                  <span className="w-1.5 h-1.5 rounded-full bg-emerald-400" />
                  蒸散冷却 活発
                </span>
              ) : (
                <span className="text-amber-400 flex items-center gap-1 justify-end">
                  <span className="w-1.5 h-1.5 rounded-full bg-amber-400" />
                  気孔閉鎖・熱滞留
                </span>
              )}
            </div>
            <div className="text-[10px] text-slate-500 font-mono">
              蒸散効率: {transpirationScore}%
            </div>
          </div>
        </div>

        {/* Transpiration Efficiency Bar */}
        <div className="w-full bg-slate-800/80 h-1.5 rounded-full mt-3 overflow-hidden">
          <div
            className={`h-full transition-all duration-700 rounded-full ${
              isHealthyTranspiration
                ? 'bg-gradient-to-r from-teal-500 to-emerald-400'
                : 'bg-gradient-to-r from-amber-600 to-rose-500'
            }`}
            style={{ width: `${Math.max(5, transpirationScore)}%` }}
          />
        </div>
      </div>

      {/* Section 2: 葉温 ＆ 大気微気象 (Dual Pair Display) */}
      <div className="grid grid-cols-2 gap-2.5 my-1">
        {/* Leaf Temp */}
        <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800/70 hover:border-slate-700/80 transition">
          <div className="flex items-center justify-between text-[11px] text-slate-400 mb-1">
            <span className="flex items-center gap-1">
              <Leaf className="w-3.5 h-3.5 text-emerald-400" />
              葉温 (IR)
              葉温
            </span>
            {leafTempRate !== 0 && (
              <span className="text-[9px] font-mono flex items-center text-slate-400">
                {leafTempRate > 0 ? (
                  <ArrowUpRight className="w-3 h-3 text-amber-400" />
                ) : (
                  <ArrowDownRight className="w-3 h-3 text-emerald-400" />
                )}
                {Math.abs(leafTempRate).toFixed(1)}/h
              </span>
            )}
          </div>
          <div className="flex items-baseline gap-1">
            <span className="text-2xl font-black text-white font-mono">
              {leafTemp !== undefined ? leafTemp.toFixed(1) : '--'}
            </span>
            <span className="text-xs text-slate-400 font-semibold">&deg;C</span>
          </div>
          <div className="text-[9px] text-slate-500 mt-1">赤外線非接触</div>
          <div className="text-[9px] text-slate-500 mt-1">植物生体表面</div>
        </div>

        {/* Air Temp */}
        <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800/70 hover:border-slate-700/80 transition">
          <div className="flex items-center justify-between text-[11px] text-slate-400 mb-1">
            <span className="flex items-center gap-1">
              <Thermometer className="w-3.5 h-3.5 text-sky-400" />
              気温 (Air)
              気温 (環境)
            </span>
            <span className="text-[9px] font-mono text-slate-500">BME280</span>
          </div>
          <div className="flex items-baseline gap-1">
            <span className="text-2xl font-black text-white font-mono">
              {airTemp !== undefined ? airTemp.toFixed(1) : '--'}
            </span>
            <span className="text-xs text-slate-400 font-semibold">&deg;C</span>
          </div>
          <div className="text-[9px] text-slate-500 mt-1">
            {airTemp >= 18 && airTemp <= 28 ? '適温帯維持' : '温度注意'}
          </div>
        </div>

        {/* Humidity */}
        <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800/70 hover:border-slate-700/80 transition">
          <div className="flex items-center justify-between text-[11px] text-slate-400 mb-1">
            <span className="flex items-center gap-1">
              <Droplets className="w-3.5 h-3.5 text-indigo-400" />
              湿度
            </span>
            <span className="text-[9px] font-mono text-slate-500">BME280</span>
          </div>
          <div className="flex items-baseline gap-1">
            <span className="text-2xl font-black text-white font-mono">
              {humidity !== undefined ? humidity.toFixed(0) : '--'}
            </span>
            <span className="text-xs text-slate-400 font-semibold">%</span>
          </div>
          <div className="text-[9px] text-slate-500 mt-1">
            {humidity >= 45 && humidity <= 70 ? '快適湿度' : '乾燥注意'}
          </div>
        </div>

        {/* Illuminance */}
        <div className="p-3 rounded-xl bg-slate-900/60 border border-slate-800/70 hover:border-slate-700/80 transition">
          <div className="flex items-center justify-between text-[11px] text-slate-400 mb-1">
            <span className="flex items-center gap-1">
              <SunMedium className="w-3.5 h-3.5 text-amber-400" />
              照度
            </span>
            <span className="text-[9px] font-mono text-slate-500">SEN0228</span>
          </div>
          <div className="flex items-baseline gap-1">
            <span className="text-2xl font-black text-amber-400 font-mono">
              {lux !== undefined ? lux : '--'}
            </span>
            <span className="text-xs text-slate-400 font-semibold">lx</span>
          </div>
          <div className="text-[9px] text-slate-500 mt-1 truncate">{luxZoneLabel}</div>
        </div>
      </div>

      {/* Section 3: 光合成受光スライダー & 土壌変化率 */}
      <div className="mt-2 pt-2.5 border-t border-slate-800/80 space-y-1.5">
        <div className="flex items-center justify-between text-[10px] text-slate-400 font-mono">
          <span>光合成光量スペクトル</span>
          <span className="text-amber-400">{lux} / 6000 lx</span>
        </div>
        <div className="w-full bg-slate-900 h-1 rounded-full overflow-hidden">
          <div
            className="h-full bg-gradient-to-r from-amber-600 via-yellow-400 to-amber-200 transition-all duration-700"
            style={{ width: `${luxBarPercent}%` }}
          />
        </div>
        {soilRate !== 0 && (
          <div className="flex items-center justify-between text-[9px] text-slate-500 font-mono pt-1">
            <span>土壌水分変化率:</span>
            <span className={soilRate < 0 ? 'text-teal-400' : 'text-slate-400'}>
              {soilRate > 0 ? '+' : ''}{soilRate.toFixed(1)}/h
            </span>
          </div>
        )}
      </div>
    </div>
  );
};
