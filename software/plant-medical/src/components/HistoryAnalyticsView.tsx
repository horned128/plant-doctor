import React, { useState, useEffect, useRef, useMemo } from 'react';
import Plotly from 'plotly.js-dist-min';
import { HistorySampleRecord, TimeResolution } from '../types';
import {
  Download,
  RotateCw,
  Trash2,
  Calendar,
  LineChart,
  Table as TableIcon,
  Columns,
  Thermometer,
  Leaf,
  Droplets,
  SunMedium,
  Activity,
  FileCode2,
} from 'lucide-react';

interface HistoryAnalyticsViewProps {
  records: HistorySampleRecord[];
  resolution: TimeResolution;
  onChangeResolution: (res: TimeResolution) => void;
  onRefresh: () => void;
  onClear: () => void;
  onExportCsv: () => void;
  onExportParquet: () => void;
  isLoading?: boolean;
}

type ViewMode = 'split' | 'chart' | 'table';

export const HistoryAnalyticsView: React.FC<HistoryAnalyticsViewProps> = ({
  records,
  resolution,
  onChangeResolution,
  onRefresh,
  onClear,
  onExportCsv,
  onExportParquet,
  isLoading = false,
}) => {
  const chartContainerRef = useRef<HTMLDivElement>(null);
  const [viewMode, setViewMode] = useState<ViewMode>('split');
  const [page, setPage] = useState(1);
  const [pageSize, setPageSize] = useState(15);
  const [statusFilter, setStatusFilter] = useState<string>('all');

  // 1. 期間フィルタリング（時・日・週・月）
  const filteredRecords = useMemo(() => {
    if (records.length === 0) return [];
    const now = Math.floor(Date.now() / 1000);
    const latestTime = records[records.length - 1]?.timestamp || now;
    let cutoffSec = 24 * 3600; // default 24h

    if (resolution === '1h') cutoffSec = 3600;
    else if (resolution === '24h') cutoffSec = 24 * 3600;
    else if (resolution === '7d') cutoffSec = 7 * 24 * 3600;
    else if (resolution === '30d') cutoffSec = 30 * 24 * 3600;

    const filtered = records.filter((r) => r.timestamp >= latestTime - cutoffSec);
    return filtered.length > 0 ? filtered : records;
  }, [records, resolution]);

  // テーブル用のステータス絞り込み
  const tableRecords = useMemo(() => {
    if (statusFilter === 'all') return filteredRecords;
    return filteredRecords.filter((r) => r.status === statusFilter);
  }, [filteredRecords, statusFilter]);

  // 2. 期間内のサマリー統計
  const stats = useMemo(() => {
    if (filteredRecords.length === 0) {
      return {
        count: 0,
        avgAirTemp: 0,
        avgLeafTemp: 0,
        avgDiff: 0,
        avgHumidity: 0,
        avgLux: 0,
        avgSoil: 0,
        avgStress: 0,
        wateringEvents: 0,
      };
    }
    const count = filteredRecords.length;
    let sumAir = 0;
    let sumLeaf = 0;
    let sumDiff = 0;
    let sumHum = 0;
    let sumLux = 0;
    let sumSoil = 0;
    let sumStress = 0;
    let wateringCount = 0;

    filteredRecords.forEach((r) => {
      sumAir += r.air_temp || 0;
      sumLeaf += r.leaf_temp || 0;
      sumDiff += r.leaf_air_diff || 0;
      sumHum += r.humidity || 0;
      sumLux += r.lux || 0;
      sumSoil += r.soil_raw || 0;
      sumStress += r.stress || 0;
      if (r.status === 'WATERING' || r.pump_on) wateringCount++;
    });

    return {
      count,
      avgAirTemp: Number((sumAir / count).toFixed(1)),
      avgLeafTemp: Number((sumLeaf / count).toFixed(1)),
      avgDiff: Number((sumDiff / count).toFixed(2)),
      avgHumidity: Number((sumHum / count).toFixed(1)),
      avgLux: Math.round(sumLux / count),
      avgSoil: Math.round(sumSoil / count),
      avgStress: Math.round(sumStress / count),
      wateringEvents: wateringCount,
    };
  }, [filteredRecords]);

  // 3. Plotly 時系列グラフの描画（実在センサ限定・日本語表記）
  useEffect(() => {
    if (!chartContainerRef.current || viewMode === 'table') return;

    const timeLabels = filteredRecords.map((r) => {
      const d = new Date(r.timestamp * 1000);
      const pad = (n: number) => n.toString().padStart(2, '0');
      if (resolution === '1h') {
        return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
      } else if (resolution === '24h') {
        return `${pad(d.getHours())}:${pad(d.getMinutes())}\n${pad(d.getMonth() + 1)}/${pad(d.getDate())}`;
      }
      return `${pad(d.getMonth() + 1)}/${pad(d.getDate())}\n${pad(d.getHours())}:00`;
    });

    const airTemps = filteredRecords.map((r) => r.air_temp);
    const leafTemps = filteredRecords.map((r) => r.leaf_temp);
    const deltaTs = filteredRecords.map((r) => r.leaf_air_diff);
    const hums = filteredRecords.map((r) => r.humidity);
    const lights = filteredRecords.map((r) => r.lux);
    const soils = filteredRecords.map((r) => r.soil_raw);
    const stresses = filteredRecords.map((r) => r.stress);
    const losses = filteredRecords.map((r) => (typeof r.ai_loss === 'number' ? r.ai_loss : 0.0215));

    const bgPaper = 'rgba(0, 0, 0, 0)';
    const bgPlot = 'rgba(15, 23, 42, 0.75)';
    const gridColor = 'rgba(51, 65, 85, 0.35)';
    const axisLineColor = 'rgba(71, 85, 105, 0.5)';
    const textColor = '#94a3b8';
    const titleColor = '#f8fafc';

    // 実在するセンサ 6サブプロット（2列 x 3行）
    const traces: Plotly.Data[] = [
      // 1. 気温 & 葉温 (℃)
      {
        x: timeLabels,
        y: airTemps,
        name: '気温',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#f43f5e', width: 2 },
        xaxis: 'x',
        yaxis: 'y',
        hovertemplate: '気温: %{y:.1f}℃<extra></extra>',
      },
      {
        x: timeLabels,
        y: leafTemps,
        name: '葉温',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#fb923c', width: 1.8, dash: 'dot' },
        xaxis: 'x',
        yaxis: 'y',
        hovertemplate: '葉温: %{y:.1f}℃<extra></extra>',
      },

      // 2. 葉温−気温差 ΔT (℃) - 蒸散冷却指標
      {
        x: timeLabels,
        y: deltaTs,
        name: '葉温-気温差 ΔT',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#2dd4bf', width: 2 },
        xaxis: 'x2',
        yaxis: 'y2',
        hovertemplate: 'ΔT: %{y:.2f}℃<extra></extra>',
      },

      // 3. 湿度 (%RH)
      {
        x: timeLabels,
        y: hums,
        name: '湿度',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#38bdf8', width: 2 },
        xaxis: 'x3',
        yaxis: 'y3',
        hovertemplate: '湿度: %{y:.1f}%<extra></extra>',
      },

      // 4. 日照・照度 (lx)
      {
        x: timeLabels,
        y: lights,
        name: '照度',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#fbbf24', width: 2 },
        xaxis: 'x4',
        yaxis: 'y4',
        hovertemplate: '照度: %{y} lx<extra></extra>',
      },

      // 5. 土壌水分 (Raw)
      {
        x: timeLabels,
        y: soils,
        name: '土壌水分Raw',
        type: 'scatter',
        mode: 'lines+markers',
        marker: { size: 3, color: '#10b981' },
        line: { color: '#10b981', width: 2 },
        xaxis: 'x5',
        yaxis: 'y5',
        hovertemplate: '土壌Raw: %{y}<extra></extra>',
      },

      // 6. 植物ストレス度 (0〜100) & AI再構成誤差
      {
        x: timeLabels,
        y: stresses,
        name: 'ストレス度',
        type: 'scatter',
        mode: 'lines+markers',
        marker: { size: 3, color: '#c084fc' },
        line: { color: '#c084fc', width: 2 },
        xaxis: 'x6',
        yaxis: 'y6',
        hovertemplate: 'ストレス: %{y}/100<extra></extra>',
      },
      {
        x: timeLabels,
        y: losses.map((l) => Math.round(l * 1000)),
        name: 'AI再構成損失 (x1000)',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#a855f7', width: 1.2, dash: 'dash' },
        xaxis: 'x6',
        yaxis: 'y6',
        hovertemplate: 'AI損失: %{y}<extra></extra>',
      },
    ];

    const commonAxis = {
      tickangle: -30,
      gridcolor: gridColor,
      zerolinecolor: gridColor,
      showline: true,
      linecolor: axisLineColor,
      tickfont: { size: 9, color: textColor, family: 'monospace' },
    };

    const commonYAxis = (title: string) => ({
      title: { text: title, font: { size: 10, color: textColor } },
      gridcolor: gridColor,
      zerolinecolor: gridColor,
      showline: true,
      linecolor: axisLineColor,
      tickfont: { size: 9, color: textColor, family: 'monospace' },
    });

    const layout: any = {
      grid: {
        rows: 3,
        columns: 2,
        pattern: 'independent',
        roworder: 'top to bottom',
      },
      showlegend: false,
      margin: { l: 55, r: 25, t: 35, b: 50 },
      paper_bgcolor: bgPaper,
      plot_bgcolor: bgPlot,
      font: { family: 'system-ui, -apple-system, sans-serif', color: textColor },

      xaxis: commonAxis,
      yaxis: commonYAxis('気温・葉温 (℃)'),

      xaxis2: commonAxis,
      yaxis2: commonYAxis('葉温−気温差 ΔT (℃)'),

      xaxis3: commonAxis,
      yaxis3: commonYAxis('湿度 (%RH)'),

      xaxis4: commonAxis,
      yaxis4: commonYAxis('日照・照度 (lx)'),

      xaxis5: commonAxis,
      yaxis5: commonYAxis('土壌水分 (Raw)'),

      xaxis6: commonAxis,
      yaxis6: commonYAxis('ストレス度 (0-100)'),

      annotations: [
        {
          text: '<b>気温・葉温</b> <span style="color:#f43f5e;">― 気温</span> <span style="color:#fb923c;">-- 葉温</span>',
          x: 0.22,
          y: 1.03,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
        {
          text: '<b>葉温−気温差 ΔT (蒸散冷却指標)</b> <span style="color:#2dd4bf;">●</span>',
          x: 0.78,
          y: 1.03,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
        {
          text: '<b>湿度 (%RH)</b> <span style="color:#38bdf8;">●</span>',
          x: 0.22,
          y: 0.66,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
        {
          text: '<b>日照・照度 (lx)</b> <span style="color:#fbbf24;">●</span>',
          x: 0.78,
          y: 0.66,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
        {
          text: '<b>土壌水分 (Raw)</b> <span style="color:#10b981;">●</span>',
          x: 0.22,
          y: 0.29,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
        {
          text: '<b>植物ストレス度 ＆ AI再構成損失</b> <span style="color:#c084fc;">●</span>',
          x: 0.78,
          y: 0.29,
          xref: 'paper',
          yref: 'paper',
          showarrow: false,
          font: { size: 12, color: titleColor },
        },
      ],
    };

    const config: any = {
      responsive: true,
      displayModeBar: true,
      displaylogo: false,
      modeBarButtonsToRemove: ['lasso2d', 'select2d'],
    };

    Plotly.newPlot(chartContainerRef.current, traces, layout, config);

    const handleResize = () => {
      if (chartContainerRef.current) {
        Plotly.Plots.resize(chartContainerRef.current);
      }
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
    };
  }, [filteredRecords, resolution, viewMode]);

  // ページネーション計算
  const totalPages = Math.max(1, Math.ceil(tableRecords.length / pageSize));
  const currentPageRecords = useMemo(() => {
    const start = (page - 1) * pageSize;
    return tableRecords.slice(start, start + pageSize);
  }, [tableRecords, page, pageSize]);

  return (
    <div className="w-full space-y-4">
      {/* 1. 統合コントロールバー (期間選択・表示切替・エクスポート) */}
      <div className="bg-slate-900/90 border border-slate-800 rounded-3xl p-4 shadow-xl flex flex-wrap items-center justify-between gap-4">
        {/* 左側: 時・日・週・月 期間セレクター */}
        <div className="flex flex-wrap items-center gap-2">
          <div className="flex items-center gap-1.5 text-xs font-bold text-slate-300 mr-2">
            <Calendar className="w-4 h-4 text-emerald-400" />
            <span>集計期間:</span>
          </div>

          <div className="flex items-center bg-slate-950/80 p-1 rounded-2xl border border-slate-800">
            <button
              onClick={() => {
                onChangeResolution('1h');
                setPage(1);
              }}
              className={`px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
                resolution === '1h'
                  ? 'bg-emerald-600 text-white shadow-md shadow-emerald-600/30'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              1時間 (時)
            </button>
            <button
              onClick={() => {
                onChangeResolution('24h');
                setPage(1);
              }}
              className={`px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
                resolution === '24h'
                  ? 'bg-emerald-600 text-white shadow-md shadow-emerald-600/30'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              24時間 (日)
            </button>
            <button
              onClick={() => {
                onChangeResolution('7d');
                setPage(1);
              }}
              className={`px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
                resolution === '7d'
                  ? 'bg-emerald-600 text-white shadow-md shadow-emerald-600/30'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              7日間 (週)
            </button>
            <button
              onClick={() => {
                onChangeResolution('30d');
                setPage(1);
              }}
              className={`px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
                resolution === '30d'
                  ? 'bg-emerald-600 text-white shadow-md shadow-emerald-600/30'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              30日間 (月)
            </button>
          </div>
        </div>

        {/* 中央: 表示モード切替 (分割 / グラフのみ / ログのみ) */}
        <div className="flex items-center bg-slate-950/80 p-1 rounded-2xl border border-slate-800">
          <button
            onClick={() => setViewMode('split')}
            className={`flex items-center space-x-1 px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
              viewMode === 'split'
                ? 'bg-teal-600 text-white shadow-md shadow-teal-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
            title="グラフとログテーブルを上下に同時表示"
          >
            <Columns className="w-3.5 h-3.5" />
            <span>グラフ ＆ ログ</span>
          </button>
          <button
            onClick={() => setViewMode('chart')}
            className={`flex items-center space-x-1 px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
              viewMode === 'chart'
                ? 'bg-teal-600 text-white shadow-md shadow-teal-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
            title="グラフのみを全画面表示"
          >
            <LineChart className="w-3.5 h-3.5" />
            <span>グラフ全画面</span>
          </button>
          <button
            onClick={() => setViewMode('table')}
            className={`flex items-center space-x-1 px-3 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
              viewMode === 'table'
                ? 'bg-teal-600 text-white shadow-md shadow-teal-600/30'
                : 'text-slate-400 hover:text-slate-200'
            }`}
            title="ログテーブルのみを全画面表示"
          >
            <TableIcon className="w-3.5 h-3.5" />
            <span>ログ一覧</span>
          </button>
        </div>

        {/* 右側: 更新・出力・消去アクション */}
        <div className="flex flex-wrap items-center gap-2">
          {/* CSV Download */}
          <button
            onClick={onExportCsv}
            className="flex items-center space-x-1 px-2.5 py-1.5 rounded-xl text-xs font-bold bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition cursor-pointer"
            title="CSV形式でエクスポート"
          >
            <Download className="w-3.5 h-3.5 text-emerald-400" />
            <span>CSV出力</span>
          </button>

          {/* Parquet Download */}
          <button
            onClick={onExportParquet}
            className="flex items-center space-x-1 px-2.5 py-1.5 rounded-xl text-xs font-bold bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition cursor-pointer"
            title="高圧縮Parquet形式でエクスポート"
          >
            <FileCode2 className="w-3.5 h-3.5 text-sky-400" />
            <span>Parquet</span>
          </button>

          {/* Refresh */}
          <button
            onClick={onRefresh}
            disabled={isLoading}
            className="p-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition cursor-pointer"
            title="最新データを再取得"
          >
            <RotateCw className={`w-4 h-4 ${isLoading ? 'animate-spin text-emerald-400' : ''}`} />
          </button>

          {/* Clear */}
          <button
            onClick={onClear}
            className="p-1.5 rounded-xl bg-slate-800/80 hover:bg-rose-950/80 text-slate-400 hover:text-rose-400 border border-slate-700 hover:border-rose-800 transition cursor-pointer"
            title="履歴ログを消去"
          >
            <Trash2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* 2. 期間内のサマリー指標カードバー */}
      <div className="grid grid-cols-2 sm:grid-cols-4 lg:grid-cols-7 gap-3">
        {/* サンプル数 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium">サンプリング点数</div>
          <div className="text-xl font-black text-white font-mono mt-0.5">
            {stats.count} <span className="text-[10px] text-slate-500 font-normal">pts</span>
          </div>
        </div>

        {/* 平均気温 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <Thermometer className="w-3 h-3 text-sky-400" />
            平均気温
          </div>
          <div className="text-xl font-black text-white font-mono mt-0.5">
            {stats.avgAirTemp} <span className="text-xs font-semibold text-slate-400">&deg;C</span>
          </div>
        </div>

        {/* 平均葉温 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <Leaf className="w-3 h-3 text-emerald-400" />
            平均葉温
          </div>
          <div className="text-xl font-black text-emerald-400 font-mono mt-0.5">
            {stats.avgLeafTemp} <span className="text-xs font-semibold text-slate-400">&deg;C</span>
          </div>
        </div>

        {/* 平均ΔT */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <Activity className="w-3 h-3 text-teal-400" />
            平均蒸散差 ΔT
          </div>
          <div
            className={`text-xl font-black font-mono mt-0.5 ${
              stats.avgDiff < 0 ? 'text-teal-300' : 'text-amber-400'
            }`}
          >
            {stats.avgDiff > 0 ? '+' : ''}{stats.avgDiff}{' '}
            <span className="text-xs font-semibold text-slate-400">&deg;C</span>
          </div>
        </div>

        {/* 平均湿度 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <Droplets className="w-3 h-3 text-indigo-400" />
            平均湿度
          </div>
          <div className="text-xl font-black text-white font-mono mt-0.5">
            {stats.avgHumidity} <span className="text-xs font-semibold text-slate-400">%</span>
          </div>
        </div>

        {/* 平均照度 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <SunMedium className="w-3 h-3 text-amber-400" />
            平均照度
          </div>
          <div className="text-xl font-black text-amber-400 font-mono mt-0.5">
            {stats.avgLux} <span className="text-xs font-semibold text-slate-400">lx</span>
          </div>
        </div>

        {/* 給水回数 */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-2xl p-3 shadow-md">
          <div className="text-[10px] text-slate-400 font-medium flex items-center gap-1">
            <Droplets className="w-3 h-3 text-sky-400" />
            給水イベント
          </div>
          <div className="text-xl font-black text-sky-400 font-mono mt-0.5">
            {stats.wateringEvents} <span className="text-[10px] text-slate-400">回</span>
          </div>
        </div>
      </div>

      {/* 3. グラフ領域（'split' または 'chart' モードで表示） */}
      {(viewMode === 'split' || viewMode === 'chart') && (
        <div className="bg-slate-900/90 border border-slate-800 rounded-3xl p-4 shadow-xl relative">
          <div className="flex items-center justify-between mb-2 px-2">
            <div className="flex items-center gap-2">
              <LineChart className="w-4 h-4 text-emerald-400" />
              <h3 className="text-sm font-bold text-white tracking-wide">
                環境 ＆ 生体時系列推移（実在センサ 6軸プロット）
              </h3>
            </div>
            <span className="text-[10px] font-mono text-slate-400">
              ROHM ML63Q2557 生体計測データ
            </span>
          </div>

          <div
            ref={chartContainerRef}
            className={`w-full ${viewMode === 'chart' ? 'h-[750px]' : 'h-[520px]'}`}
          />
        </div>
      )}

      {/* 4. 履歴ログテーブル領域（'split' または 'table' モードで表示） */}
      {(viewMode === 'split' || viewMode === 'table') && (
        <div className="bg-slate-900/90 border border-slate-800 rounded-3xl p-5 shadow-xl space-y-3">
          {/* テーブル上部バー */}
          <div className="flex flex-wrap items-center justify-between gap-3 border-b border-slate-800 pb-3">
            <div className="flex items-center gap-2">
              <TableIcon className="w-4 h-4 text-teal-400" />
              <h3 className="text-sm font-bold text-white tracking-wide">
                10分サンプリング履歴ログ一覧（{tableRecords.length}件）
              </h3>
            </div>

            {/* 状態フィルター & ページサイズ */}
            <div className="flex items-center gap-3 text-xs">
              <div className="flex items-center gap-1.5">
                <span className="text-slate-400">表示件数:</span>
                <select
                  value={pageSize}
                  onChange={(e) => {
                    setPageSize(Number(e.target.value));
                    setPage(1);
                  }}
                  className="bg-slate-800 border border-slate-700 text-slate-200 text-xs rounded-xl px-2 py-1 focus:outline-none focus:border-emerald-500 cursor-pointer"
                >
                  <option value={15}>15件</option>
                  <option value={30}>30件</option>
                  <option value={50}>50件</option>
                </select>
              </div>

              <div className="flex items-center gap-1.5">
                <span className="text-slate-400">診断絞り込み:</span>
                <select
                  value={statusFilter}
                  onChange={(e) => {
                    setStatusFilter(e.target.value);
                    setPage(1);
                  }}
                  className="bg-slate-800 border border-slate-700 text-slate-200 text-xs rounded-xl px-2.5 py-1 focus:outline-none focus:border-emerald-500 cursor-pointer"
                >
                  <option value="all">すべての診断結果</option>
                  <option value="HEALTHY">正常・健康 (Healthy)</option>
                  <option value="DRY_STRESS">乾燥ストレス (Dry)</option>
                  <option value="HEAT_STRESS">熱ストレス (Heat)</option>
                  <option value="WATERING">給水中 (Watering)</option>
                  <option value="WATERING_FAILED">給水失敗 (Failed)</option>
                </select>
              </div>
            </div>
          </div>

          {/* テーブル本体 */}
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs text-slate-300">
              <thead className="text-[11px] font-bold text-slate-400 bg-slate-950/60 uppercase border-b border-slate-800">
                <tr>
                  <th className="py-2.5 px-3">サンプリング日時</th>
                  <th className="py-2.5 px-3">診断ステータス</th>
                  <th className="py-2.5 px-3">ストレス度</th>
                  <th className="py-2.5 px-3">気温 (Air)</th>
                  <th className="py-2.5 px-3">葉温 (Leaf)</th>
                  <th className="py-2.5 px-3">ΔT (蒸散差)</th>
                  <th className="py-2.5 px-3">湿度</th>
                  <th className="py-2.5 px-3">照度</th>
                  <th className="py-2.5 px-3">土壌Raw</th>
                  <th className="py-2.5 px-3">給水動作</th>
                  <th className="py-2.5 px-3">AI損失 (MSE)</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/60 font-mono">
                {currentPageRecords.length > 0 ? (
                  currentPageRecords.map((r, idx) => {
                    const date = new Date(r.timestamp * 1000);
                    const isHealthy = r.status === 'HEALTHY';
                    const isWatering = r.status === 'WATERING' || r.pump_on;
                    const isDry = r.status === 'DRY_STRESS';
                    const isHeat = r.status === 'HEAT_STRESS';

                    return (
                      <tr key={idx} className="hover:bg-slate-800/50 transition">
                        <td className="py-2.5 px-3 text-slate-300 whitespace-nowrap">
                          {date.toLocaleDateString('ja-JP')}{' '}
                          <span className="text-slate-400">
                            {date.toLocaleTimeString('ja-JP')}
                          </span>
                        </td>
                        <td className="py-2.5 px-3 whitespace-nowrap font-sans font-semibold">
                          {isHealthy ? (
                            <span className="px-2 py-0.5 rounded-full text-[10px] bg-emerald-950 text-emerald-400 border border-emerald-800">
                              正常・健康
                            </span>
                          ) : isWatering ? (
                            <span className="px-2 py-0.5 rounded-full text-[10px] bg-sky-950 text-sky-400 border border-sky-800 animate-pulse">
                              給水動作中
                            </span>
                          ) : isDry ? (
                            <span className="px-2 py-0.5 rounded-full text-[10px] bg-amber-950 text-amber-400 border border-amber-800">
                              乾燥ストレス
                            </span>
                          ) : isHeat ? (
                            <span className="px-2 py-0.5 rounded-full text-[10px] bg-orange-950 text-orange-400 border border-orange-800">
                              熱ストレス
                            </span>
                          ) : (
                            <span className="px-2 py-0.5 rounded-full text-[10px] bg-slate-800 text-slate-400">
                              {r.status}
                            </span>
                          )}
                        </td>
                        <td className="py-2.5 px-3 font-bold text-white">
                          <span
                            className={
                              r.stress >= 50
                                ? 'text-rose-400'
                                : r.stress >= 25
                                ? 'text-amber-400'
                                : 'text-emerald-400'
                            }
                          >
                            {r.stress}
                          </span>
                          <span className="text-slate-500 text-[10px]">/100</span>
                        </td>
                        <td className="py-2.5 px-3 text-white">{r.air_temp?.toFixed(1)}℃</td>
                        <td className="py-2.5 px-3 text-emerald-400 font-bold">
                          {r.leaf_temp?.toFixed(1)}℃
                        </td>
                        <td
                          className={`py-2.5 px-3 font-bold ${
                            r.leaf_air_diff < 0 ? 'text-teal-400' : 'text-amber-400'
                          }`}
                        >
                          {r.leaf_air_diff > 0 ? '+' : ''}
                          {r.leaf_air_diff?.toFixed(2)}℃
                        </td>
                        <td className="py-2.5 px-3 text-slate-300">{r.humidity?.toFixed(0)}%</td>
                        <td className="py-2.5 px-3 text-amber-400">{r.lux} lx</td>
                        <td className="py-2.5 px-3 text-white">{r.soil_raw}</td>
                        <td className="py-2.5 px-3 whitespace-nowrap">
                          {r.pump_on ? (
                            <span className="text-sky-400 font-bold flex items-center gap-1">
                              <span className="w-1.5 h-1.5 rounded-full bg-sky-400 animate-ping" />
                              ON (駆動)
                            </span>
                          ) : (
                            <span className="text-slate-500">待機</span>
                          )}
                        </td>
                        <td className="py-2.5 px-3 text-cyan-400 font-bold">
                          {typeof r.ai_loss === 'number' ? r.ai_loss.toFixed(4) : '--'}
                        </td>
                      </tr>
                    );
                  })
                ) : (
                  <tr>
                    <td colSpan={11} className="py-8 text-center text-slate-500">
                      指定期間のデータが存在しないか、受信待機中です。
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>

          {/* ページネーション */}
          {totalPages > 1 && (
            <div className="flex items-center justify-between pt-3 border-t border-slate-800 text-xs">
              <span className="text-slate-400 font-mono">
                ページ {page} / {totalPages} (全{tableRecords.length}件)
              </span>
              <div className="flex items-center gap-1.5">
                <button
                  onClick={() => setPage((p) => Math.max(1, p - 1))}
                  disabled={page === 1}
                  className="px-3 py-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 disabled:opacity-40 transition cursor-pointer"
                >
                  前へ
                </button>
                <button
                  onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
                  disabled={page === totalPages}
                  className="px-3 py-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 disabled:opacity-40 transition cursor-pointer"
                >
                  次へ
                </button>
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  );
};
