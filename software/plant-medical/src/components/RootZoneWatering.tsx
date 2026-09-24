import React, { useState } from 'react';
import { Layers, Droplets, AlertTriangle, Clock, History, CheckCircle2, XCircle } from 'lucide-react';
import { WateringEventLog } from '../types';

interface RootZoneWateringProps {
  soilRaw: number;
  soilTrend: string;
  tankLiquid: boolean;
  pumpOn: boolean;
  lastWateringLog?: WateringEventLog;
  wateringLogs?: WateringEventLog[];
  onTriggerWatering: (durationSec: number) => void;
}

export const RootZoneWatering: React.FC<RootZoneWateringProps> = ({
  soilRaw,
  soilTrend,
  tankLiquid,
  pumpOn,
  lastWateringLog,
  wateringLogs = [],
  onTriggerWatering,
}) => {
  const [selectedDuration, setSelectedDuration] = useState<number>(10);
  const [showHistoryModal, setShowHistoryModal] = useState<boolean>(false);

  // Soil Raw normalization: ~1200 wet (100%), ~2400 dry (0%)
  const soilMoisturePercent = Math.min(
    100,
    Math.max(0, Math.round(((2400 - soilRaw) / (2400 - 1200)) * 100))
  );

  const isSoilOptimal = soilRaw < 2100;

  const formatTimeAgo = (timestampSec: number) => {
    const diff = Math.floor(Date.now() / 1000) - timestampSec;
    if (diff < 60) return `${diff}秒前`;
    if (diff < 3600) return `${Math.floor(diff / 60)}分前`;
    if (diff < 86400) return `${Math.floor(diff / 3600)}時間前`;
    return `${Math.floor(diff / 86400)}日前`;
  };

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
              <span className="text-[10px] text-slate-400 bg-slate-900/80 px-2 py-0.5 rounded border border-slate-800">
                土壌環境
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
                <span className="text-emerald-400">最適水分域</span>
              ) : (
                <span className="text-amber-400">水分低下・給水推奨</span>
              )}
            </span>
            <span className="text-slate-500 font-sans">
              トレンド: {soilTrend === 'RISING' ? '上昇中' : soilTrend === 'FALLING' ? '乾燥傾向' : '安定'}
            </span>
          </div>
          <div className="w-full bg-slate-800/80 h-2 rounded-full overflow-hidden">
            <div
              className={`h-full transition-all duration-500 rounded-full ${
                isSoilOptimal
                  ? 'bg-gradient-to-r from-teal-500 to-emerald-400'
                  : 'bg-gradient-to-r from-amber-600 to-rose-500'
              }`}
              style={{ width: `${soilMoisturePercent}%` }}
            />
          </div>
        </div>
      </div>

      {/* Middle: Last Watering Log Badge */}
      <div className="flex items-center gap-2 bg-slate-900/70 border border-slate-800/80 px-3 py-1.5 rounded-2xl text-xs">
        <Clock className="w-4 h-4 text-teal-400 shrink-0" />
        <div className="flex flex-col">
          <span className="text-[10px] text-slate-400">直近の給水実績</span>
          {lastWateringLog ? (
            <div className="flex items-center gap-1.5 font-mono text-[11px]">
              <span className="text-white font-bold">{lastWateringLog.durationSec}秒</span>
              {lastWateringLog.success ? (
                <span className="text-emerald-400 flex items-center gap-0.5">
                  <CheckCircle2 className="w-3 h-3" /> 成功
                </span>
              ) : (
                <span className="text-rose-400 flex items-center gap-0.5">
                  <XCircle className="w-3 h-3" /> 失敗
                </span>
              )}
              <span className="text-slate-500">({formatTimeAgo(lastWateringLog.timestamp)})</span>
            </div>
          ) : (
            <span className="text-slate-500 text-[11px]">記録なし</span>
          )}
        </div>
        <button
          onClick={() => setShowHistoryModal(true)}
          className="ml-1 p-1 hover:bg-slate-800 rounded-lg text-slate-400 hover:text-slate-200 transition cursor-pointer"
          title="給水履歴一覧を表示"
        >
          <History className="w-3.5 h-3.5" />
        </button>
      </div>

      {/* Right: Tank Safety Interlock & Direct Watering Trigger */}
      <div className="flex items-center space-x-3 w-full sm:w-auto justify-end">
        {/* Tank Level Badge */}
        <div className="flex items-center space-x-2 bg-slate-900/80 px-3 py-1.5 rounded-2xl border border-slate-800 text-xs">
          <Droplets
            className={`w-4 h-4 ${
              tankLiquid ? 'text-teal-400 animate-pulse' : 'text-rose-400'
            }`}
          />
          <div className="flex flex-col">
            <span className="text-[9px] text-slate-400 leading-tight">給水タンク</span>
            <span
              className={`font-bold text-[11px] ${
                tankLiquid ? 'text-teal-300' : 'text-rose-400 flex items-center gap-1'
              }`}
            >
              {!tankLiquid && <AlertTriangle className="w-3 h-3" />}
              {tankLiquid ? '水位OK' : 'タンク空 (給水停止)'}
            </span>
          </div>
        </div>

        {/* Duration Select */}
        <div className="flex items-center bg-slate-900/90 border border-slate-800 rounded-2xl px-2 py-1 text-xs">
          <span className="text-slate-400 text-[11px] mr-1.5">秒数:</span>
          <select
            value={selectedDuration}
            onChange={(e) => setSelectedDuration(Number(e.target.value))}
            className="bg-slate-950 text-white font-mono text-xs rounded px-1.5 py-0.5 border border-slate-700 outline-none focus:border-teal-400 cursor-pointer"
          >
            <option value={5}>5秒 (少: 約10ml)</option>
            <option value={8}>8秒 (約16ml)</option>
            <option value={10}>10秒 (標準: 約20ml)</option>
            <option value={15}>15秒 (約30ml)</option>
            <option value={20}>20秒 (多: 約40ml)</option>
          </select>
        </div>

        {/* Manual Water Button */}
        <button
          onClick={() => onTriggerWatering(selectedDuration)}
          disabled={pumpOn || !tankLiquid}
          className={`flex items-center space-x-2 px-4 py-2 rounded-2xl text-xs font-black transition-all cursor-pointer shadow-lg active:scale-95 ${
            pumpOn
              ? 'bg-teal-500 text-slate-950 animate-pulse shadow-teal-500/30'
              : !tankLiquid
              ? 'bg-slate-800 text-slate-500 cursor-not-allowed border border-slate-700/50'
              : 'bg-gradient-to-r from-teal-500 to-emerald-400 hover:from-teal-400 hover:to-emerald-300 text-slate-950 shadow-emerald-500/20'
          }`}
        >
          <Droplets className="w-4 h-4 fill-current" />
          <span>{pumpOn ? '給水ポンプ稼働中...' : `手動給水 (${selectedDuration}秒)`}</span>
        </button>
      </div>

      {/* History Modal */}
      {showHistoryModal && (
        <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-3xl w-full max-w-lg p-5 shadow-2xl flex flex-col max-h-[85vh]">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3 mb-3">
              <div className="flex items-center gap-2">
                <History className="w-5 h-5 text-teal-400" />
                <h3 className="font-bold text-white text-sm">給水実績ログ履歴 (直近)</h3>
              </div>
              <button
                onClick={() => setShowHistoryModal(false)}
                className="text-slate-400 hover:text-white px-2 py-1 rounded-lg text-sm cursor-pointer"
              >
                ✕
              </button>
            </div>

            <div className="overflow-y-auto flex-1 space-y-2 pr-1">
              {wateringLogs.length === 0 ? (
                <div className="text-center py-8 text-slate-500 text-xs">給水履歴がありません</div>
              ) : (
                wateringLogs.map((log) => (
                  <div
                    key={log.id}
                    className="p-3 rounded-2xl bg-slate-950/80 border border-slate-800/80 flex items-center justify-between text-xs"
                  >
                    <div className="flex items-center gap-2.5">
                      {log.success ? (
                        <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0" />
                      ) : (
                        <XCircle className="w-4 h-4 text-rose-400 shrink-0" />
                      )}
                      <div>
                        <div className="flex items-center gap-2">
                          <span className="font-bold text-white font-mono">{log.durationSec}秒給水</span>
                          <span className={`text-[10px] px-1.5 py-0.2 rounded font-bold ${
                            log.success
                              ? 'bg-emerald-950 text-emerald-300 border border-emerald-800'
                              : 'bg-rose-950 text-rose-300 border border-rose-800'
                          }`}>
                            {log.reason || (log.success ? '正常完了' : '失敗')}
                          </span>
                        </div>
                        <div className="text-[10px] text-slate-400 mt-0.5">
                          {new Date(log.timestamp * 1000).toLocaleString('ja-JP')}
                        </div>
                      </div>
                    </div>

                    <div className="text-right text-[11px] font-mono">
                      {log.soilBefore !== undefined && log.soilAfter !== undefined ? (
                        <div className="text-slate-300">
                          {log.soilBefore} &rarr; <span className="text-teal-400">{log.soilAfter}</span>
                        </div>
                      ) : null}
                    </div>
                  </div>
                ))
              )}
            </div>

            <div className="mt-4 pt-3 border-t border-slate-800 flex justify-end">
              <button
                onClick={() => setShowHistoryModal(false)}
                className="px-4 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold cursor-pointer"
              >
                閉じる
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
