import React from 'react';
import { Thermometer, Droplets, Leaf, Activity, Layers, SunMedium } from 'lucide-react';

interface MetricGridProps {
  airTemp: number;
  humidity: number;
  leafTemp: number;
  leafAirDiff: number;
  soilRaw: number;
  lux: number;
}

export const MetricGrid: React.FC<MetricGridProps> = ({
  airTemp,
  humidity,
  leafTemp,
  leafAirDiff,
  soilRaw,
  lux,
}) => {
  const diffSign = leafAirDiff >= 0 ? '+' : '';
  const isHealthyTranspiration = leafAirDiff < 0;

  return (
    <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-3">
      {/* 1. 気温 */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <Thermometer className="w-3.5 h-3.5 text-sky-400" />
            気温
          </span>
          <span className="text-[9px] font-mono text-slate-500">BME280</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span className="text-2xl font-black text-white tracking-tight">
            {airTemp !== undefined ? airTemp.toFixed(1) : '--'}
          </span>
          <span className="text-xs text-slate-400 font-semibold">&deg;C</span>
        </div>
        <div className="text-[10px] text-slate-400 truncate">
          {airTemp >= 18 && airTemp <= 28 ? '🌱 生育適温' : '⚠️ 温度注意'}
        </div>
      </div>

      {/* 2. 湿度 */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <Droplets className="w-3.5 h-3.5 text-indigo-400" />
            湿度
          </span>
          <span className="text-[9px] font-mono text-slate-500">BME280</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span className="text-2xl font-black text-white tracking-tight">
            {humidity !== undefined ? humidity.toFixed(0) : '--'}
          </span>
          <span className="text-xs text-slate-400 font-semibold">%</span>
        </div>
        <div className="text-[10px] text-slate-400 truncate">
          {humidity >= 40 && humidity <= 75 ? '💧 適正湿度' : '乾燥または多湿'}
        </div>
      </div>

      {/* 3. 葉温 */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <Leaf className="w-3.5 h-3.5 text-emerald-400" />
            葉温
          </span>
          <span className="text-[9px] font-mono text-slate-500">SEN0206</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span className="text-2xl font-black text-emerald-400 tracking-tight">
            {leafTemp !== undefined ? leafTemp.toFixed(1) : '--'}
          </span>
          <span className="text-xs text-slate-400 font-semibold">&deg;C</span>
        </div>
        <div className="text-[10px] text-emerald-400/90 truncate">
          非接触赤外線生体計測
        </div>
      </div>

      {/* 4. 葉温−気温差 (ΔT) */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <Activity className="w-3.5 h-3.5 text-teal-400" />
            葉温&minus;気温差 (&Delta;T)
          </span>
          <span className="text-[9px] font-mono text-slate-500">蒸散指標</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span
            className={`text-2xl font-black tracking-tight ${
              isHealthyTranspiration ? 'text-emerald-400' : 'text-amber-400'
            }`}
          >
            {diffSign}{leafAirDiff !== undefined ? leafAirDiff.toFixed(2) : '--'}
          </span>
          <span className="text-xs text-slate-400 font-semibold">&deg;C</span>
        </div>
        <div className="text-[10px] text-slate-400 truncate">
          {isHealthyTranspiration ? '🍃 蒸散冷却中 (健全)' : '気孔閉鎖・熱滞留'}
        </div>
      </div>

      {/* 5. 土壌水分Raw */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <Layers className="w-3.5 h-3.5 text-amber-500" />
            土壌水分Raw
          </span>
          <span className="text-[9px] font-mono text-slate-500">SEN0193</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span className="text-2xl font-black text-white tracking-tight">
            {soilRaw !== undefined ? soilRaw : '--'}
          </span>
          <span className="text-[10px] text-slate-500 font-mono">/4095</span>
        </div>
        <div className="text-[10px] text-slate-400 truncate">
          {soilRaw < 2100 ? '水分充分 (OK)' : '水分低下 (給水推奨)'}
        </div>
      </div>

      {/* 6. 照度 */}
      <div className="bg-slate-900/80 border border-slate-800/80 rounded-xl p-3.5 shadow-md flex flex-col justify-between hover:border-slate-700 transition">
        <div className="flex items-center justify-between text-slate-400 mb-1">
          <span className="text-[11px] font-medium flex items-center gap-1">
            <SunMedium className="w-3.5 h-3.5 text-yellow-400" />
            日照・照度
          </span>
          <span className="text-[9px] font-mono text-slate-500">SEN0228</span>
        </div>
        <div className="flex items-baseline gap-1 my-1">
          <span className="text-2xl font-black text-amber-400 tracking-tight">
            {lux !== undefined ? lux : '--'}
          </span>
          <span className="text-xs text-slate-400 font-semibold">lux</span>
        </div>
        <div className="text-[10px] text-slate-400 truncate">
          {lux > 100 ? '☀️ 光合成稼働域' : '🌙 夜間・弱光域'}
        </div>
      </div>
    </div>
  );
};
