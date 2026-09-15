import React, { useState, useMemo } from 'react';
import { HistorySampleRecord, HistoryRange } from '../types';
import { exportHistoryToCsv } from '../services/historyApi';
import {
  Download,
  RotateCw,
  Trash2,
  Server,
} from 'lucide-react';

interface HistoryLogTableProps {
  records: HistorySampleRecord[];
  range: HistoryRange;
  onChangeRange: (range: HistoryRange) => void;
  onRefresh: () => void;
  onClear: () => void;
  isLoading?: boolean;
}

export const HistoryLogTable: React.FC<HistoryLogTableProps> = ({
  records,
  range,
  onChangeRange,
  onRefresh,
  onClear,
  isLoading = false,
}) => {
  const [page, setPage] = useState(1);
  const pageSize = 15;

  // Filter records by range
  const filteredRecords = useMemo(() => {
    if (records.length === 0) return [];
    const now = Math.floor(Date.now() / 1000);
    let cutoff = 0;

    if (range === '1h') cutoff = now - 3600;
    else if (range === '6h') cutoff = now - 6 * 3600;
    else if (range === '24h') cutoff = now - 24 * 3600;
    else if (range === '7d') cutoff = now - 7 * 24 * 3600;
    else cutoff = 0; // 'all'

    // If records are simulated/past timestamps, ensure we filter reasonably
    const latestTime = records[records.length - 1]?.timestamp || now;
    if (range === '1h') cutoff = latestTime - 3600;
    else if (range === '6h') cutoff = latestTime - 6 * 3600;
    else if (range === '24h') cutoff = latestTime - 24 * 3600;
    else if (range === '7d') cutoff = latestTime - 7 * 24 * 3600;
    else cutoff = 0;

    return records.filter((r) => r.timestamp >= cutoff);
  }, [records, range]);

  // Statistics Summary
  const stats = useMemo(() => {
    if (filteredRecords.length === 0) {
      return { count: 0, avgTemp: 0, avgLeaf: 0, avgStress: 0, wateringEvents: 0 };
    }

    const count = filteredRecords.length;
    let sumTemp = 0;
    let sumLeaf = 0;
    let sumStress = 0;
    let wateringCount = 0;

    filteredRecords.forEach((r) => {
      sumTemp += r.air_temp || 0;
      sumLeaf += r.leaf_temp || 0;
      sumStress += r.stress || 0;
      if (r.status === 'WATERING' || r.pump_on) {
        wateringCount++;
      }
    });

    return {
      count,
      avgTemp: Number((sumTemp / count).toFixed(1)),
      avgLeaf: Number((sumLeaf / count).toFixed(1)),
      avgStress: Math.round(sumStress / count),
      wateringEvents: wateringCount,
    };
  }, [filteredRecords]);

  // Paginated records (sorted newest first)
  const sortedNewestFirst = useMemo(() => {
    return [...filteredRecords].reverse();
  }, [filteredRecords]);

  const totalPages = Math.ceil(sortedNewestFirst.length / pageSize) || 1;
  const currentRecords = useMemo(() => {
    const start = (page - 1) * pageSize;
    return sortedNewestFirst.slice(start, start + pageSize);
  }, [sortedNewestFirst, page]);

  const ranges: { id: HistoryRange; label: string }[] = [
    { id: '1h', label: '1時間' },
    { id: '6h', label: '6時間' },
    { id: '24h', label: '24時間' },
    { id: '7d', label: '7日間' },
    { id: 'all', label: '全期間' },
  ];

  return (
    <div className="bg-slate-900/90 border border-slate-800/80 rounded-2xl p-5 shadow-xl space-y-4">
      {/* Header & Controls Toolbar */}
      <div className="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 pb-4 border-b border-slate-800">
        <div className="flex items-center space-x-3">
          <div className="p-2 rounded-xl bg-indigo-500/10 border border-indigo-500/20 text-indigo-400">
            <Server className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h3 className="text-base font-bold text-white tracking-wide">
                サーバ保存 10分サンプリング履歴ログ
              </h3>
              <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-indigo-950 text-indigo-300 border border-indigo-800">
                10-min Resampled
              </span>
            </div>
            <p className="text-xs text-slate-400">
              ATOMS3 Lite / ゲートウェイに蓄積された植物生体・環境データをどこからでも閲覧可能
            </p>
          </div>
        </div>

        {/* Action Buttons & Filter */}
        <div className="flex flex-wrap items-center gap-2.5">
          {/* Range Buttons */}
          <div className="flex items-center bg-slate-950 p-1 rounded-xl border border-slate-800 text-xs">
            {ranges.map((r) => (
              <button
                key={r.id}
                onClick={() => {
                  onChangeRange(r.id);
                  setPage(1);
                }}
                className={`px-2.5 py-1 rounded-lg font-medium transition ${
                  range === r.id
                    ? 'bg-indigo-600 text-white font-bold shadow'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {r.label}
              </button>
            ))}
          </div>

          {/* Refresh */}
          <button
            onClick={onRefresh}
            disabled={isLoading}
            className="p-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl border border-slate-700 transition"
            title="サーバから最新ログを再取得"
          >
            <RotateCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
          </button>

          {/* CSV Export */}
          <button
            onClick={() => exportHistoryToCsv(filteredRecords)}
            disabled={filteredRecords.length === 0}
            className="flex items-center space-x-1.5 px-3 py-1.5 bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-bold rounded-xl shadow transition"
          >
            <Download className="w-3.5 h-3.5" />
            <span>CSV出力</span>
          </button>

          {/* Clear */}
          <button
            onClick={onClear}
            className="p-2 bg-slate-800 hover:bg-rose-900/40 text-slate-400 hover:text-rose-400 rounded-xl border border-slate-700 transition"
            title="ログを消去"
          >
            <Trash2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Summary Statistics Ribbon */}
      <div className="grid grid-cols-2 sm:grid-cols-5 gap-3 bg-slate-950/60 p-3 rounded-xl border border-slate-800/80 text-xs">
        <div>
          <span className="text-slate-500 text-[10px]">記録サンプル数</span>
          <div className="text-base font-bold text-white mt-0.5">
            {stats.count} <span className="text-[10px] text-slate-400 font-normal">件</span>
          </div>
        </div>
        <div>
          <span className="text-slate-500 text-[10px]">平均気温</span>
          <div className="text-base font-bold text-sky-400 mt-0.5">
            {stats.avgTemp} &deg;C
          </div>
        </div>
        <div>
          <span className="text-slate-500 text-[10px]">平均葉温</span>
          <div className="text-base font-bold text-emerald-400 mt-0.5">
            {stats.avgLeaf} &deg;C
          </div>
        </div>
        <div>
          <span className="text-slate-500 text-[10px]">平均ストレス度</span>
          <div className="text-base font-bold text-amber-400 mt-0.5">
            {stats.avgStress} <span className="text-[10px] text-slate-400 font-normal">/100</span>
          </div>
        </div>
        <div>
          <span className="text-slate-500 text-[10px]">給水検知回数</span>
          <div className="text-base font-bold text-indigo-400 mt-0.5">
            {stats.wateringEvents} <span className="text-[10px] text-slate-400 font-normal">回</span>
          </div>
        </div>
      </div>

      {/* Main Table */}
      <div className="overflow-x-auto rounded-xl border border-slate-800">
        <table className="w-full text-left text-xs border-collapse">
          <thead>
            <tr className="bg-slate-950/80 text-slate-400 border-b border-slate-800 text-[11px] font-semibold">
              <th className="py-2.5 px-3">日時 (JST)</th>
              <th className="py-2.5 px-3">AI 診断</th>
              <th className="py-2.5 px-3">ストレス度</th>
              <th className="py-2.5 px-3">気温 / 湿度</th>
              <th className="py-2.5 px-3">葉温 (&Delta;T)</th>
              <th className="py-2.5 px-3">土壌水分</th>
              <th className="py-2.5 px-3">照度</th>
              <th className="py-2.5 px-3">タンク / ポンプ</th>
              <th className="py-2.5 px-3">AI Loss</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/60 font-mono">
            {currentRecords.length === 0 ? (
              <tr>
                <td colSpan={9} className="py-8 text-center text-slate-500 font-sans">
                  記録された10分サンプリングログがありません。
                </td>
              </tr>
            ) : (
              currentRecords.map((r, i) => {
                const date = new Date(r.timestamp * 1000);
                const timeStr = `${(date.getMonth() + 1).toString().padStart(2, '0')}/${date
                  .getDate()
                  .toString()
                  .padStart(2, '0')} ${date
                  .getHours()
                  .toString()
                  .padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}`;

                const diffSign = r.leaf_air_diff >= 0 ? '+' : '';

                return (
                  <tr
                    key={r.timestamp + '-' + i}
                    className="hover:bg-slate-800/40 transition-colors"
                  >
                    {/* Timestamp */}
                    <td className="py-2 px-3 text-slate-300 font-sans">{timeStr}</td>

                    {/* Diagnosis Status */}
                    <td className="py-2 px-3 font-sans">
                      <span
                        className={`px-2 py-0.5 rounded text-[10px] font-bold ${
                          r.status === 'HEALTHY'
                            ? 'bg-emerald-950 text-emerald-400 border border-emerald-800'
                            : r.status === 'WATERING'
                            ? 'bg-sky-950 text-sky-400 border border-sky-800'
                            : r.status === 'DRY_STRESS'
                            ? 'bg-orange-950 text-orange-400 border border-orange-800'
                            : r.status === 'HEAT_STRESS'
                            ? 'bg-amber-950 text-amber-400 border border-amber-800'
                            : 'bg-rose-950 text-rose-400 border border-rose-800'
                        }`}
                      >
                        {r.status}
                      </span>
                    </td>

                    {/* Stress Score */}
                    <td className="py-2 px-3">
                      <div className="flex items-center space-x-2">
                        <span
                          className={`font-bold ${
                            r.stress >= 60
                              ? 'text-rose-400'
                              : r.stress >= 30
                              ? 'text-amber-400'
                              : 'text-emerald-400'
                          }`}
                        >
                          {r.stress}
                        </span>
                        <div className="w-12 h-1.5 bg-slate-800 rounded-full overflow-hidden">
                          <div
                            className={`h-full ${
                              r.stress >= 60
                                ? 'bg-rose-500'
                                : r.stress >= 30
                                ? 'bg-amber-500'
                                : 'bg-emerald-500'
                            }`}
                            style={{ width: `${Math.min(100, r.stress)}%` }}
                          />
                        </div>
                      </div>
                    </td>

                    {/* Air Temp / Humidity */}
                    <td className="py-2 px-3 text-slate-300">
                      {r.air_temp.toFixed(1)}&deg;C / {r.humidity.toFixed(0)}%
                    </td>

                    {/* Leaf Temp / DeltaT */}
                    <td className="py-2 px-3">
                      <span className="text-emerald-400">{r.leaf_temp.toFixed(1)}&deg;C</span>
                      <span
                        className={`text-[10px] ml-1.5 ${
                          r.leaf_air_diff < 0 ? 'text-teal-400' : 'text-amber-400'
                        }`}
                      >
                        ({diffSign}{r.leaf_air_diff.toFixed(2)})
                      </span>
                    </td>

                    {/* Soil Raw */}
                    <td className="py-2 px-3 text-slate-300">
                      {r.soil_raw}{' '}
                      <span className="text-[10px] text-slate-500 font-sans">
                        ({r.soil_trend || 'STABLE'})
                      </span>
                    </td>

                    {/* Lux */}
                    <td className="py-2 px-3 text-amber-300">{r.lux} lx</td>

                    {/* Tank & Pump */}
                    <td className="py-2 px-3 font-sans text-[11px]">
                      {r.pump_on ? (
                        <span className="text-sky-400 font-bold">💧 給水中</span>
                      ) : r.tank_liquid ? (
                        <span className="text-emerald-400">満水</span>
                      ) : (
                        <span className="text-rose-400 font-bold">空</span>
                      )}
                    </td>

                    {/* AI Loss */}
                    <td className="py-2 px-3 text-slate-400">
                      {r.ai_loss !== undefined ? r.ai_loss.toFixed(4) : '--'}
                    </td>
                  </tr>
                );
              })
            )}
          </tbody>
        </table>
      </div>

      {/* Pagination Footer */}
      {totalPages > 1 && (
        <div className="flex items-center justify-between text-xs text-slate-400 pt-2">
          <span>
            {sortedNewestFirst.length} 件中 {(page - 1) * pageSize + 1} 〜{' '}
            {Math.min(page * pageSize, sortedNewestFirst.length)} 件を表示
          </span>
          <div className="flex items-center space-x-1">
            <button
              onClick={() => setPage((p) => Math.max(1, p - 1))}
              disabled={page === 1}
              className="px-2.5 py-1 rounded bg-slate-800 text-slate-300 disabled:opacity-40 hover:bg-slate-700"
            >
              前へ
            </button>
            <span className="px-2 font-mono">
              {page} / {totalPages}
            </span>
            <button
              onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
              disabled={page === totalPages}
              className="px-2.5 py-1 rounded bg-slate-800 text-slate-300 disabled:opacity-40 hover:bg-slate-700"
            >
              次へ
            </button>
          </div>
        </div>
      )}
    </div>
  );
};
