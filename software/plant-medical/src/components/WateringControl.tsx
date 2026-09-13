import React from 'react';

interface WateringControlProps {
  tankLiquid: boolean;
  pumpOn: boolean;
  demoMode: boolean;
  onTriggerWatering: () => void;
  onToggleDemo: (enable: boolean) => void;
}

export const WateringControl: React.FC<WateringControlProps> = ({
  tankLiquid,
  pumpOn,
  demoMode,
  onTriggerWatering,
  onToggleDemo,
}) => {
  return (
    <div className="bg-slate-900/90 rounded-2xl p-6 border border-slate-800 shadow-xl flex flex-col justify-between">
      <div>
        <div className="flex justify-between items-center mb-3">
          <span className="text-xs font-bold text-slate-400 uppercase tracking-wider">給水制御 &bull; 安全インターロック</span>
          <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-slate-800 text-slate-400">
            R4 / R5
          </span>
        </div>

        <div className="flex items-center space-x-2 my-2">
          {tankLiquid ? (
            <span className="px-3 py-1.5 rounded-xl text-xs font-bold bg-emerald-950 text-emerald-400 border border-emerald-800 flex items-center gap-1.5">
              <span className="w-2 h-2 rounded-full bg-emerald-400" />
              タンク満水 (Liquid OK)
            </span>
          ) : (
            <span className="px-3 py-1.5 rounded-xl text-xs font-bold bg-rose-950 text-rose-400 border border-rose-800 flex items-center gap-1.5 animate-pulse">
              <span className="w-2 h-2 rounded-full bg-rose-500" />
              タンク空 (給水拒否)
            </span>
          )}

          {pumpOn ? (
            <span className="px-3 py-1.5 rounded-xl text-xs font-bold bg-sky-950 text-sky-400 border border-sky-800 flex items-center gap-1.5 animate-pulse">
              <span className="w-2 h-2 rounded-full bg-sky-400 animate-ping" />
              ポンプ駆動中 (ON)
            </span>
          ) : (
            <span className="px-3 py-1.5 rounded-xl text-xs font-bold bg-slate-800 text-slate-400 border border-slate-700">
              ポンプ停止 (Standby)
            </span>
          )}
        </div>

        <p className="text-[11px] text-slate-400 mt-3 leading-normal">
          最大駆動2.0秒 &bull; クールダウン3.0秒 &bull; 液面なし自動停止がDT-EBML側でハードウェア保護されています。
        </p>
      </div>

      <div className="mt-6 pt-4 border-t border-slate-800/80 space-y-3">
        <button
          onClick={onTriggerWatering}
          disabled={!tankLiquid}
          className={`w-full font-bold py-2.5 px-4 rounded-xl shadow-lg transition flex items-center justify-center space-x-2 text-xs ${
            tankLiquid
              ? 'bg-sky-600 hover:bg-sky-500 text-white shadow-sky-600/20 active:scale-95'
              : 'bg-slate-800 text-slate-500 cursor-not-allowed border border-slate-700'
          }`}
        >
          <span>💧 手動給水テスト (SW4等価)</span>
        </button>

        <div className="flex items-center justify-between text-xs text-slate-400 px-1">
          <span className="flex items-center gap-1.5">
            <span className="w-2 h-2 rounded-full bg-indigo-400" />
            デモモード (DEMO 1〜5 再現)
          </span>
          <label className="relative inline-flex items-center cursor-pointer">
            <input
              type="checkbox"
              checked={demoMode}
              onChange={(e) => onToggleDemo(e.target.checked)}
              className="sr-only peer"
            />
            <div className="w-9 h-5 bg-slate-700 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-slate-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all peer-checked:bg-indigo-600" />
          </label>
        </div>
      </div>
    </div>
  );
};
