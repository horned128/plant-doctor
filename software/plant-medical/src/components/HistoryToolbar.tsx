import React from 'react';
import { HistoryRange, DatabaseStats } from '../types';
import {
  Clock,
  Radio,
  History,
  Database,
  Download,
  Trash2,
  Pause,
  Play,
  RotateCcw,
} from 'lucide-react';

interface HistoryToolbarProps {
  mode: 'live' | 'history';
  onToggleMode: (mode: 'live' | 'history') => void;
  historyRange: HistoryRange;
  onChangeRange: (range: HistoryRange) => void;
  stats: DatabaseStats;
  isRecording: boolean;
  onToggleRecording: () => void;
  onClearHistory: () => void;
  onExportHistoryCsv: () => void;
  scrubberIndex: number;
  scrubberMax: number;
  onScrub: (index: number) => void;
  scrubbedTimeStr?: string;
}

export const HistoryToolbar: React.FC<HistoryToolbarProps> = ({
  mode,
  onToggleMode,
  historyRange,
  onChangeRange,
  stats,
  isRecording,
  onToggleRecording,
  onClearHistory,
  onExportHistoryCsv,
  scrubberIndex,
  scrubberMax,
  onScrub,
  scrubbedTimeStr,
}) => {
  const ranges: { id: HistoryRange; label: string }[] = [
    { id: '10m', label: '10分' },
    { id: '1h', label: '1時間' },
    { id: '6h', label: '6時間' },
    { id: '24h', label: '24時間' },
    { id: 'all', label: '全期間' },
  ];

  return (
    <div className="bg-slate-900/90 border border-slate-700/80 rounded-2xl p-4 shadow-xl backdrop-blur-sm mb-6">
      {/* Top Bar: Mode Switch + Storage Info + Action Buttons */}
      <div className="flex flex-wrap items-center justify-between gap-4">
        {/* Mode Toggle Switch */}
        <div className="flex items-center bg-slate-950 p-1 rounded-xl border border-slate-800">
          <button
            onClick={() => onToggleMode('live')}
            className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-lg text-xs font-bold transition-all ${
              mode === 'live'
                ? 'bg-emerald-600 text-white shadow-lg shadow-emerald-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <Radio className={`w-3.5 h-3.5 ${mode === 'live' ? 'animate-pulse' : ''}`} />
            <span>リアルタイム監視 (Live)</span>
          </button>
          <button
            onClick={() => onToggleMode('history')}
            className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-lg text-xs font-bold transition-all ${
              mode === 'history'
                ? 'bg-indigo-600 text-white shadow-lg shadow-indigo-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            <History className="w-3.5 h-3.5" />
            <span>過去ログ閲覧 (History)</span>
          </button>
        </div>

        {/* Range Selector (Active in History mode) */}
        {mode === 'history' && (
          <div className="flex items-center bg-slate-950 p-1 rounded-xl border border-slate-800">
            <span className="text-xs text-slate-500 px-2 flex items-center gap-1 font-mono">
              <Clock className="w-3 h-3" />
              範囲:
            </span>
            {ranges.map((r) => (
              <button
                key={r.id}
                onClick={() => onChangeRange(r.id)}
                className={`px-3 py-1 rounded-lg text-xs font-medium transition-all ${
                  historyRange === r.id
                    ? 'bg-indigo-500/20 text-indigo-300 border border-indigo-500/40 font-bold'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {r.label}
              </button>
            ))}
          </div>
        )}

        {/* Storage Stats & Controls */}
        <div className="flex flex-wrap items-center gap-2 text-xs">
          {/* DB Record Count Badge */}
          <div className="flex items-center space-x-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800 text-slate-300 font-mono">
            <Database className="w-3.5 h-3.5 text-cyan-400" />
            <span>
              蓄積: <strong className="text-white">{stats.totalRecords.toLocaleString()}</strong> 件
            </span>
            <span className="text-slate-500 text-[11px]">({stats.estimatedSizeMb} MB)</span>
          </div>

          {/* Recording Toggle Button */}
          <button
            onClick={onToggleRecording}
            title={isRecording ? '記録を一時停止' : '記録を再開'}
            className={`flex items-center space-x-1 px-2.5 py-1.5 rounded-xl border font-medium transition ${
              isRecording
                ? 'bg-slate-800/80 hover:bg-slate-700/80 border-slate-700 text-emerald-400'
                : 'bg-amber-950/60 hover:bg-amber-900/60 border-amber-500/50 text-amber-300 animate-pulse'
            }`}
          >
            {isRecording ? (
              <>
                <Pause className="w-3 h-3" />
                <span>記録中</span>
              </>
            ) : (
              <>
                <Play className="w-3 h-3" />
                <span>停止中</span>
              </>
            )}
          </button>

          {/* Export CSV Button */}
          <button
            onClick={onExportHistoryCsv}
            title="全期間の学習・生体データをCSV形式でダウンロード"
            className="flex items-center space-x-1 bg-slate-800 hover:bg-slate-700 px-2.5 py-1.5 rounded-xl border border-slate-700 text-slate-200 transition active:scale-95"
          >
            <Download className="w-3 h-3 text-cyan-400" />
            <span>全CSV出力</span>
          </button>

          {/* Clear DB Button */}
          <button
            onClick={onClearHistory}
            title="ブラウザ内に保存された過去ログを全消去"
            className="flex items-center space-x-1 bg-slate-800 hover:bg-rose-950/60 px-2.5 py-1.5 rounded-xl border border-slate-700 hover:border-rose-700/50 text-slate-400 hover:text-rose-300 transition active:scale-95"
          >
            <Trash2 className="w-3 h-3" />
            <span>ログ消去</span>
          </button>
        </div>
      </div>

      {/* History Timeline Scrubber (Rendered in History Mode) */}
      {mode === 'history' && scrubberMax > 0 && (
        <div className="mt-4 pt-3 border-t border-slate-800/80">
          <div className="flex flex-wrap items-center justify-between text-xs mb-2">
            <div className="flex items-center space-x-2">
              <span className="font-bold text-indigo-300 flex items-center gap-1.5">
                <History className="w-4 h-4" />
                タイムライン再生・生体スナップショット再現:
              </span>
              <span className="font-mono text-white bg-indigo-950/80 border border-indigo-500/40 px-2 py-0.5 rounded text-[11px]">
                {scrubbedTimeStr || '選択なし'}
              </span>
            </div>
            <div className="flex items-center space-x-2 text-slate-400 font-mono text-[11px]">
              <span>位置: {scrubberIndex + 1} / {scrubberMax + 1}</span>
              <button
                onClick={() => onScrub(scrubberMax)}
                className="text-xs text-indigo-400 hover:text-indigo-300 flex items-center gap-1"
              >
                <RotateCcw className="w-3 h-3" />
                最新へ戻る
              </button>
            </div>
          </div>

          {/* Slider bar */}
          <div className="relative flex items-center">
            <input
              type="range"
              min={0}
              max={scrubberMax}
              value={scrubberIndex}
              onChange={(e) => onScrub(parseInt(e.target.value, 10))}
              className="w-full h-2 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-indigo-500 hover:accent-indigo-400 transition"
            />
          </div>
          <div className="flex justify-between text-[10px] font-mono text-slate-500 mt-1">
            <span>◀ 過去の起点</span>
            <span>スライダーを動かすと、上部の全生体計器・8次元レーダー・AI損失がその瞬間の状態にタイムトラベルします</span>
            <span>最新の終点 ▶</span>
          </div>
        </div>
      )}
    </div>
  );
};
