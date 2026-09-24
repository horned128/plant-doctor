import React, { useState, useMemo } from 'react';
import { HistorySampleRecord, HistoryRange } from '../types';
import { exportHistoryToParquet, exportHistoryToCsv } from '../services/parquetExporter';
import {
  downloadServerParquetFile,
  downloadServerCsvFile,
} from '../services/historyApi';
import {
  Download,
  RotateCw,
  Trash2,
  Server,
  Filter,
  Info,
  CheckCircle2,
  AlertTriangle,
  Droplets,
  Flame,
  FileCode2,
  HardDrive,
  FolderCheck,
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
  const [pageSize, setPageSize] = useState(15);
  const [statusFilter, setStatusFilter] = useState<string>('all');
  const [stressFilter, setStressFilter] = useState<string>('all');
  const [showServerStorageInfo, setShowServerStorageInfo] = useState(false);

  // 1. 期間フィルタリング
  const rangeFilteredRecords = useMemo(() => {
    if (records.length === 0) return [];
    const now = Math.floor(Date.now() / 1000);
    const latestTime = records[records.length - 1]?.timestamp || now;
    let cutoff = 0;

    if (range === '1h') cutoff = latestTime - 3600;
    else if (range === '6h') cutoff = latestTime - 6 * 3600;
    else if (range === '24h') cutoff = latestTime - 24 * 3600;
    else if (range === '7d') cutoff = latestTime - 7 * 24 * 3600;
    else cutoff = 0;

    return records.filter((r) => r.timestamp >= cutoff);
  }, [records, range]);

  // 2. 状態・ストレス複合フィルタリング
  const filteredRecords = useMemo(() => {
    return rangeFilteredRecords.filter((r) => {
      if (statusFilter !== 'all' && r.status !== statusFilter) return false;
      if (stressFilter === 'healthy' && r.stress >= 25) return false;
      if (stressFilter === 'warning' && (r.stress < 25 || r.stress >= 60)) return false;
      if (stressFilter === 'critical' && r.stress < 60) return false;
      return true;
    });
  }, [rangeFilteredRecords, statusFilter, stressFilter]);

  // 3. サマリー統計
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

  // 降順ソート（最新が一番上）
  const sortedNewestFirst = useMemo(() => {
    return [...filteredRecords].reverse();
  }, [filteredRecords]);

  const totalPages = Math.ceil(sortedNewestFirst.length / pageSize) || 1;
  const currentRecords = useMemo(() => {
    const start = (page - 1) * pageSize;
    return sortedNewestFirst.slice(start, start + pageSize);
  }, [sortedNewestFirst, page, pageSize]);

  const ranges: { id: HistoryRange; label: string }[] = [
    { id: '1h', label: '1時間' },
    { id: '6h', label: '6時間' },
    { id: '24h', label: '24時間' },
    { id: '7d', label: '7日間' },
    { id: 'all', label: '全件' },
  ];

  // ダウンロードハンドラ: サーバPCファイルAPIを試し、失敗時はローカル生成
  const handleDownloadParquet = () => {
    try {
      downloadServerParquetFile();
    } catch {
      exportHistoryToParquet(filteredRecords);
    }
  };

  const handleDownloadCsv = () => {
    try {
      downloadServerCsvFile();
    } catch {
      exportHistoryToCsv(filteredRecords);
    }
  };

  return (
    <div className="bg-slate-900/90 border border-slate-800/80 rounded-2xl p-5 shadow-xl space-y-4">
      {/* ツールバー: ヘッダー & エクスポート & ガイド */}
      <div className="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 pb-4 border-b border-slate-800">
        <div className="flex items-center space-x-3">
          <div className="p-2.5 rounded-xl bg-indigo-500/10 border border-indigo-500/20 text-indigo-400">
            <Server className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h3 className="text-base font-bold text-white tracking-wide">
                サーバPC保存 10分サンプリング履歴ログ
              </h3>
              <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-emerald-950 text-emerald-300 border border-emerald-800 font-mono flex items-center gap-1">
                <HardDrive className="w-3 h-3" />
                Server Disk Synced
              </span>
              <button
                onClick={() => setShowServerStorageInfo(!showServerStorageInfo)}
                className="text-slate-400 hover:text-amber-300 p-1 rounded-lg transition"
                title="サーバPC保存先ファイルについての解説"
              >
                <Info className="w-4 h-4" />
              </button>
            </div>
            <p className="text-xs text-slate-400">
              サーバPC上のファイル（<code>data/plant_history.parquet</code>, <code>.csv</code>）に直接蓄積保存
            </p>
          </div>
        </div>

        {/* アクションボタン群 */}
        <div className="flex flex-wrap items-center gap-2">
          {/* 期間切り替え */}
          <div className="flex items-center bg-slate-950 p-1 rounded-xl border border-slate-800 text-xs">
            {ranges.map((r) => (
              <button
                key={r.id}
                onClick={() => {
                  onChangeRange(r.id);
                  setPage(1);
                }}
                className={`px-2.5 py-1 rounded-lg font-medium transition cursor-pointer ${
                  range === r.id
                    ? 'bg-indigo-600 text-white font-bold shadow'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {r.label}
              </button>
            ))}
          </div>

          {/* 最新化 */}
          <button
            onClick={onRefresh}
            disabled={isLoading}
            className="p-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl border border-slate-700 transition cursor-pointer"
            title="サーバPCの最新ログファイルを同期"
          >
            <RotateCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
          </button>

          {/* Parquet 保存ボタン */}
          <button
            onClick={handleDownloadParquet}
            disabled={filteredRecords.length === 0}
            className="flex items-center space-x-1.5 px-3 py-1.5 bg-gradient-to-r from-amber-600 to-amber-500 hover:from-amber-500 hover:to-amber-400 text-slate-950 text-xs font-extrabold rounded-xl shadow-md transition cursor-pointer"
            title="サーバPCに蓄積された実ファイル plant_history.parquet を取得"
          >
            <FileCode2 className="w-3.5 h-3.5" />
            <span>Parquet保存</span>
          </button>

          {/* CSV 出力ボタン */}
          <button
            onClick={handleDownloadCsv}
            disabled={filteredRecords.length === 0}
            className="flex items-center space-x-1.5 px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-bold rounded-xl border border-slate-700 shadow transition cursor-pointer"
            title="サーバPC上の plant_history.csv を取得"
          >
            <Download className="w-3.5 h-3.5 text-sky-400" />
            <span>CSV出力</span>
          </button>

          {/* ログ消去 */}
          <button
            onClick={onClear}
            className="p-2 bg-slate-800 hover:bg-rose-900/40 text-slate-400 hover:text-rose-400 rounded-xl border border-slate-700 transition cursor-pointer"
            title="サーバPC上のログファイルをリセット"
          >
            <Trash2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* サーバ保存先についての解説アコーディオン */}
      {showServerStorageInfo && (
        <div className="bg-indigo-950/40 border border-indigo-500/40 rounded-xl p-3.5 text-xs text-indigo-200 space-y-2">
          <div className="font-bold flex items-center gap-1.5 text-indigo-300">
            <FolderCheck className="w-4 h-4 text-emerald-400" />
            <span>【保存仕様】ログデータはサーバPCのどのファイルに保存されているか？</span>
          </div>
          <p className="text-[11px] leading-relaxed text-slate-300">
            本システムでは、マイコン（ATOMS3 Lite）やブラウザの IndexedDB / localStorage には一切保存せず、
            <strong className="text-white">Webアプリを稼働させているサーバPCのローカルディスク</strong>
            に物理ファイルとして自動蓄積・追記保存されています。
          </p>
          <div className="bg-slate-950/70 p-2.5 rounded-lg border border-indigo-900/60 font-mono text-[11px] text-slate-200 space-y-1">
            <div>📁 保存先ディレクトリ: <span className="text-emerald-400">software/plant-medical/data/</span></div>
            <div>├── <span className="text-amber-400 font-bold">plant_history.parquet</span> (Apache Parquet 列指向・高圧縮バイナリ)</div>
            <div>├── <span className="text-sky-400">plant_history.csv</span> (ExcelやPandasで直接開ける標準CSV)</div>
            <div>└── <span className="text-slate-400">plant_history.json</span> (構造化RawレコードJSON)</div>
          </div>
          <p className="text-[11px] leading-relaxed text-slate-300">
            「Parquet保存」「CSV出力」ボタンを押すと、サーバPCのディスク上に蓄積されている実ファイルを瞬時にダウンロードできます。
          </p>
        </div>
      )}

      {/* 絞り込み・検索バー */}
      <div className="flex flex-wrap items-center justify-between gap-3 bg-slate-950/50 p-2.5 rounded-xl border border-slate-800/80 text-xs">
        <div className="flex flex-wrap items-center gap-2">
          <div className="flex items-center text-slate-400 font-semibold space-x-1 mr-1">
            <Filter className="w-3.5 h-3.5 text-slate-400" />
            <span>フィルタ:</span>
          </div>

          {/* 診断状態フィルタ */}
          <select
            value={statusFilter}
            onChange={(e) => {
              setStatusFilter(e.target.value);
              setPage(1);
            }}
            className="bg-slate-800 border border-slate-700 text-slate-200 rounded-lg px-2.5 py-1 text-xs focus:outline-none cursor-pointer"
          >
            <option value="all">すべての診断状態</option>
            <option value="HEALTHY">🟢 HEALTHY (健康)</option>
            <option value="WATERING">💧 WATERING (給水検知)</option>
            <option value="DRY_STRESS">🟠 DRY_STRESS (乾燥)</option>
            <option value="HEAT_STRESS">🔴 HEAT_STRESS (熱ストレス)</option>
          </select>

          {/* ストレスフィルタ */}
          <select
            value={stressFilter}
            onChange={(e) => {
              setStressFilter(e.target.value);
              setPage(1);
            }}
            className="bg-slate-800 border border-slate-700 text-slate-200 rounded-lg px-2.5 py-1 text-xs focus:outline-none cursor-pointer"
          >
            <option value="all">すべてのストレス度</option>
            <option value="healthy">安定・正常 (0〜24)</option>
            <option value="warning">要注意 (25〜59)</option>
            <option value="critical">深刻 (60〜100)</option>
          </select>
        </div>

        {/* 1ページあたりの件数切替 */}
        <div className="flex items-center space-x-2 text-slate-400 text-xs">
          <span>表示件数:</span>
          <select
            value={pageSize}
            onChange={(e) => {
              setPageSize(Number(e.target.value));
              setPage(1);
            }}
            className="bg-slate-800 border border-slate-700 text-slate-200 rounded-lg px-2 py-1 text-xs focus:outline-none cursor-pointer"
          >
            <option value={15}>15件</option>
            <option value={30}>30件</option>
            <option value={50}>50件</option>
          </select>
        </div>
      </div>

      {/* サマリー統計カード群 */}
      <div className="grid grid-cols-2 sm:grid-cols-5 gap-3 bg-slate-950/60 p-3 rounded-xl border border-slate-800/80 text-xs">
        <div>
          <span className="text-slate-500 text-[10px]">該当レコード数</span>
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

      {/* 履歴テーブル本体 */}
      <div className="overflow-x-auto rounded-xl border border-slate-800">
        <table className="w-full text-left text-xs border-collapse">
          <thead>
            <tr className="bg-slate-950/80 text-slate-400 border-b border-slate-800 text-[11px] font-semibold">
              <th className="py-2.5 px-3">日時 (JST)</th>
              <th className="py-2.5 px-3">診断状態</th>
              <th className="py-2.5 px-3">ストレス (0-100)</th>
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
                  条件に一致する10分サンプリングログがありません。
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
                    <td className="py-2 px-3 text-slate-300 font-sans">{timeStr}</td>

                    <td className="py-2 px-3 font-sans">
                      <span
                        className={`inline-flex items-center gap-1 px-2 py-0.5 rounded text-[10px] font-bold ${
                          r.status === 'HEALTHY'
                            ? 'bg-emerald-950/80 text-emerald-400 border border-emerald-800'
                            : r.status === 'WATERING'
                            ? 'bg-sky-950/80 text-sky-400 border border-sky-800'
                            : r.status === 'DRY_STRESS'
                            ? 'bg-orange-950/80 text-orange-400 border border-orange-800'
                            : r.status === 'HEAT_STRESS'
                            ? 'bg-amber-950/80 text-amber-400 border border-amber-800'
                            : 'bg-rose-950/80 text-rose-400 border border-rose-800'
                        }`}
                      >
                        {r.status === 'HEALTHY' && <CheckCircle2 className="w-2.5 h-2.5" />}
                        {r.status === 'WATERING' && <Droplets className="w-2.5 h-2.5" />}
                        {r.status === 'HEAT_STRESS' && <Flame className="w-2.5 h-2.5" />}
                        {r.status === 'DRY_STRESS' && <AlertTriangle className="w-2.5 h-2.5" />}
                        <span>{r.status}</span>
                      </span>
                    </td>

                    <td className="py-2 px-3">
                      <div className="flex items-center space-x-2">
                        <span
                          className={`font-bold ${
                            r.stress >= 60
                              ? 'text-rose-400'
                              : r.stress >= 25
                              ? 'text-amber-400'
                              : 'text-emerald-400'
                          }`}
                        >
                          {r.stress}
                        </span>
                        <div className="w-12 h-1.5 bg-slate-800 rounded-full overflow-hidden">
                          <div
                            className={`h-full transition-all ${
                              r.stress >= 60
                                ? 'bg-rose-500'
                                : r.stress >= 25
                                ? 'bg-amber-500'
                                : 'bg-emerald-500'
                            }`}
                            style={{ width: `${Math.min(100, r.stress)}%` }}
                          />
                        </div>
                      </div>
                    </td>

                    <td className="py-2 px-3 text-slate-300">
                      {r.air_temp.toFixed(1)}&deg;C / {r.humidity.toFixed(0)}%
                    </td>

                    <td className="py-2 px-3">
                      <span className="text-emerald-400">{r.leaf_temp.toFixed(1)}&deg;C</span>
                      <span
                        className={`text-[10px] ml-1.5 ${
                          r.leaf_air_diff < 0 ? 'text-teal-400 font-bold' : 'text-amber-400'
                        }`}
                      >
                        ({diffSign}{r.leaf_air_diff.toFixed(2)})
                      </span>
                    </td>

                    <td className="py-2 px-3 text-slate-300">
                      {r.soil_raw}{' '}
                      <span className="text-[10px] text-slate-500 font-sans">
                        ({r.soil_trend || 'STABLE'})
                      </span>
                    </td>

                    <td className="py-2 px-3 text-amber-300">{r.lux} lx</td>

                    <td className="py-2 px-3 font-sans text-[11px]">
                      {r.pump_on ? (
                        <span className="text-sky-400 font-bold flex items-center gap-1">
                          <Droplets className="w-3 h-3 animate-bounce" /> 給水中
                        </span>
                      ) : r.tank_liquid ? (
                        <span className="text-emerald-400">満水</span>
                      ) : (
                        <span className="text-rose-400 font-bold">空</span>
                      )}
                    </td>

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

      {/* ページネーションフッター */}
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
              className="px-2.5 py-1 rounded bg-slate-800 text-slate-300 disabled:opacity-40 hover:bg-slate-700 cursor-pointer"
            >
              前へ
            </button>
            <span className="px-2 font-mono text-slate-300 font-bold">
              {page} / {totalPages}
            </span>
            <button
              onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
              disabled={page === totalPages}
              className="px-2.5 py-1 rounded bg-slate-800 text-slate-300 disabled:opacity-40 hover:bg-slate-700 cursor-pointer"
            >
              次へ
            </button>
          </div>
        </div>
      )}
    </div>
  );
};
