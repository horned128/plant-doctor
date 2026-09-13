import React from 'react';

interface EnvironmentPanelProps {
  airTemp: number;
  humidity: number;
  leafTemp: number;
  leafAirDiff: number;
  soilRaw: number;
  lux: number;
}

export const EnvironmentPanel: React.FC<EnvironmentPanelProps> = ({
  airTemp,
  humidity,
  leafTemp,
  leafAirDiff,
  soilRaw,
  lux,
}) => {
  const diffSign = leafAirDiff >= 0 ? '+' : '';

  return (
    <div className="bg-slate-900/90 rounded-2xl p-6 border border-slate-800 shadow-xl flex flex-col justify-between">
      <div>
        <div className="flex justify-between items-center mb-4">
          <span className="text-xs font-bold text-slate-400 uppercase tracking-wider">環境 &bull; 生体センサ計測</span>
          <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-slate-800 text-slate-400">
            1秒更新
          </span>
        </div>

        <div className="grid grid-cols-2 gap-4">
          <div className="bg-slate-800/40 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[11px] text-slate-400 font-medium">葉温 (SEN0206)</span>
            <div className="text-xl font-black text-emerald-400 mt-1">
              {leafTemp !== undefined ? leafTemp.toFixed(2) : '--'}
              <span className="text-xs text-slate-400 ml-1">&deg;C</span>
            </div>
          </div>

          <div className="bg-slate-800/40 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[11px] text-slate-400 font-medium">気温 / 湿度 (BME280)</span>
            <div className="text-xl font-black text-sky-400 mt-1">
              {airTemp !== undefined ? airTemp.toFixed(1) : '--'}
              <span className="text-xs text-slate-400 ml-1">&deg;C</span>
              <span className="text-sm font-normal text-slate-400 ml-1">
                / {humidity !== undefined ? humidity.toFixed(0) : '--'}%
              </span>
            </div>
          </div>

          <div className="bg-slate-800/40 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[11px] text-slate-400 font-medium">土壌水分Raw (SEN0193)</span>
            <div className="text-xl font-black text-white mt-1">
              {soilRaw !== undefined ? soilRaw : '--'}
              <span className="text-xs text-slate-500 ml-1 font-mono">/4095</span>
            </div>
          </div>

          <div className="bg-slate-800/40 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[11px] text-slate-400 font-medium">照度Raw (SEN0228)</span>
            <div className="text-xl font-black text-amber-400 mt-1">
              {lux !== undefined ? lux : '--'}
              <span className="text-xs text-slate-500 ml-1">lux</span>
            </div>
          </div>
        </div>
      </div>

      <div className="mt-4 pt-3 border-t border-slate-800/80 flex justify-between items-center text-xs">
        <span className="text-slate-400">葉温&minus;気温差 (&Delta;T 蒸散評価):</span>
        <span
          className={`font-mono font-bold text-sm ${
            leafAirDiff < 1.0 ? 'text-emerald-400' : leafAirDiff < 2.0 ? 'text-amber-400' : 'text-rose-400'
          }`}
        >
          {diffSign}{leafAirDiff !== undefined ? leafAirDiff.toFixed(2) : '--'} &deg;C
        </span>
      </div>
    </div>
  );
};
