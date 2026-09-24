import React, { useState, useEffect, useRef, useMemo } from 'react';
import Plotly from 'plotly.js-dist-min';
import { HistorySampleRecord, TimeResolution, CustomDateRange, WateringEventLog } from '../types';
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
  Activity,
  FileCode2,
  Clock,
  CheckCircle2,
  XCircle,
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
  wateringLogs?: WateringEventLog[];
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
  wateringLogs = [],
}) => {
  const chartContainerRef = useRef<HTMLDivElement>(null);
  const [viewMode, setViewMode] = useState<ViewMode>('split');
  const [page, setPage] = useState(1);
  const [pageSize, setPageSize] = useState(15);
  const [statusFilter, setStatusFilter] = useState<string>('all');
  const [sortOrder, setSortOrder] = useState<'desc' | 'asc'>('desc');

  // Custom date picker state
  const [showDateModal, setShowDateModal] = useState<boolean>(false);
  const todayStr = useMemo(() => new Date().toISOString().slice(0, 10), []);
  const [customRange, setCustomRange] = useState<CustomDateRange>({
    startDate: todayStr,
    startHour: 0,
    endDate: todayStr,
    endHour: 23,
    singleDayOnly: false,
  });

  // 1. 期間フィルタリング（時・日・週・月・カスタムカレンダー）
  const filteredRecords = useMemo(() => {
    if (records.length === 0) return [];
    
    // タイムスタンプが有効なもののみ（不正な値 <= 1000000000 を除外）かつ昇順ソート
    const validSorted = [...records]
      .filter((r) => typeof r.timestamp === 'number' && r.timestamp > 1000000000)
      .sort((a, b) => a.timestamp - b.timestamp);

    if (validSorted.length === 0) return [];

    if (resolution === 'custom') {
      const startD = new Date(`${customRange.startDate}T${String(customRange.startHour).padStart(2, '0')}:00:00`);
      const startSec = Math.floor(startD.getTime() / 1000);
      
      let endSec: number;
      if (customRange.singleDayOnly) {
        const endD = new Date(`${customRange.startDate}T23:59:59`);
        endSec = Math.floor(endD.getTime() / 1000);
      } else {
        const endD = new Date(`${customRange.endDate}T${String(customRange.endHour).padStart(2, '0')}:59:59`);
        endSec = Math.floor(endD.getTime() / 1000);
      }
      return validSorted.filter((r) => r.timestamp >= startSec && r.timestamp <= endSec);
    }

    const latestTime = validSorted[validSorted.length - 1]?.timestamp || Math.floor(Date.now() / 1000);
    let cutoffSec = 24 * 3600; // default 24h

    if (resolution === '1h') cutoffSec = 3600;
    else if (resolution === '24h') cutoffSec = 24 * 3600;
    else if (resolution === '7d') cutoffSec = 7 * 24 * 3600;
    else if (resolution === '30d') cutoffSec = 30 * 24 * 3600;

    return validSorted.filter((r) => r.timestamp >= latestTime - cutoffSec);
  }, [records, resolution, customRange]);

  // テーブル用ステータスフィルタリング & 日付ソート（デフォルト降順）
  const tableRecords = useMemo(() => {
    const list = statusFilter === 'all'
      ? filteredRecords
      : filteredRecords.filter((r) => r.status === statusFilter);
    return [...list].sort((a, b) =>
      sortOrder === 'desc' ? b.timestamp - a.timestamp : a.timestamp - b.timestamp
    );
  }, [filteredRecords, statusFilter, sortOrder]);

  // ページネーション計算
  const totalPages = Math.ceil(tableRecords.length / pageSize) || 1;
  const pagedRecords = useMemo(() => {
    const start = (page - 1) * pageSize;
    return tableRecords.slice(start, start + pageSize);
  }, [tableRecords, page, pageSize]);

  // 統計値の算出
  const stats = useMemo(() => {
    if (filteredRecords.length === 0) {
      return {
        avgAir: 0,
        avgLeaf: 0,
        avgDiff: 0,
        minSoil: 0,
        maxSoil: 0,
        curSoil: 0,
        avgLoss: 0,
        stressCount: 0,
      };
    }
    const sumAir = filteredRecords.reduce((acc, r) => acc + (r.air_temp || 0), 0);
    const sumLeaf = filteredRecords.reduce((acc, r) => acc + (r.leaf_temp || 0), 0);
    const sumDiff = filteredRecords.reduce((acc, r) => acc + (r.leaf_air_diff || 0), 0);
    const sumLoss = filteredRecords.reduce((acc, r) => acc + (r.ai_loss || 0), 0);
    const soils = filteredRecords.map((r) => r.soil_raw || 0);
    const stressItems = filteredRecords.filter(
      (r) => r.status === 'HEAT_STRESS' || r.status === 'DRY_STRESS' || (r.stress || 0) >= 50
    );

    return {
      avgAir: Number((sumAir / filteredRecords.length).toFixed(1)),
      avgLeaf: Number((sumLeaf / filteredRecords.length).toFixed(1)),
      avgDiff: Number((sumDiff / filteredRecords.length).toFixed(2)),
      minSoil: Math.min(...soils),
      maxSoil: Math.max(...soils),
      curSoil: soils[soils.length - 1] || 0,
      avgLoss: Number((sumLoss / filteredRecords.length).toFixed(4)),
      stressCount: stressItems.length,
    };
  }, [filteredRecords]);

  // 横軸目盛りのスマート生成（時間・日・週・月）
  const generateSmartTicks = (timestamps: number[], res: TimeResolution) => {
    if (timestamps.length === 0) return { tickvals: [], ticktext: [] };

    const minTs = timestamps[0];
    const maxTs = timestamps[timestamps.length - 1];
    const tickvals: string[] = [];
    const ticktext: string[] = [];

    if (res === '1h') {
      for (const t of timestamps) {
        const d = new Date(t * 1000);
        if (d.getMinutes() % 10 === 0) {
          tickvals.push(d.toISOString());
          ticktext.push(`${d.getHours()}時${d.getMinutes() > 0 ? d.getMinutes() + '分' : ''}`);
        }
      }
    } else if (res === '24h') {
      const seenHours = new Set<number>();
      for (const t of timestamps) {
        const d = new Date(t * 1000);
        const h = d.getHours();
        if (h % 3 === 0 && !seenHours.has(h)) {
          seenHours.add(h);
          tickvals.push(d.toISOString());
          ticktext.push(`${h}時`);
        }
      }
    } else if (res === '7d') {
      const seenDays = new Set<string>();
      for (const t of timestamps) {
        const d = new Date(t * 1000);
        const dayKey = `${d.getMonth() + 1}-${d.getDate()}`;
        if (!seenDays.has(dayKey)) {
          seenDays.add(dayKey);
          tickvals.push(d.toISOString());
          ticktext.push(`${d.getDate()}日`);
        }
      }
    } else if (res === '30d') {
      const seenDays = new Set<number>();
      for (const t of timestamps) {
        const d = new Date(t * 1000);
        const day = d.getDate();
        if ((day === 1 || day % 5 === 0) && !seenDays.has(day)) {
          seenDays.add(day);
          tickvals.push(d.toISOString());
          ticktext.push(`${day}日`);
        }
      }
    } else {
      const spanDays = (maxTs - minTs) / 86400;
      if (spanDays <= 1.5) {
        const seenHours = new Set<number>();
        for (const t of timestamps) {
          const d = new Date(t * 1000);
          const h = d.getHours();
          if (h % 2 === 0 && !seenHours.has(h)) {
            seenHours.add(h);
            tickvals.push(d.toISOString());
            ticktext.push(`${h}時`);
          }
        }
      } else {
        const seenDays = new Set<string>();
        for (const t of timestamps) {
          const d = new Date(t * 1000);
          const dayKey = `${d.getMonth() + 1}-${d.getDate()}`;
          if (!seenDays.has(dayKey)) {
            seenDays.add(dayKey);
            tickvals.push(d.toISOString());
            ticktext.push(`${d.getDate()}日`);
          }
        }
      }
    }

    return { tickvals, ticktext };
  };

  // 2. Plotly.js によるマルチパネルグラフ描画 (Purge徹底・連続時系列軸)
  useEffect(() => {
    if (!chartContainerRef.current) return;

    if (filteredRecords.length === 0) {
      Plotly.purge(chartContainerRef.current);
      return;
    }

    const timestamps = filteredRecords.map((r) => r.timestamp);
    const xValues = timestamps.map((ts) => new Date(ts * 1000).toISOString());

    const leafTemps = filteredRecords.map((r) => r.leaf_temp);
    const airTemps = filteredRecords.map((r) => r.air_temp);
    const deltaTs = filteredRecords.map((r) => r.leaf_air_diff);
    const soils = filteredRecords.map((r) => r.soil_raw);
    const losses = filteredRecords.map((r) => r.ai_loss ?? 0.02);
    const stresses = filteredRecords.map((r) => r.stress);

    const { tickvals, ticktext } = generateSmartTicks(timestamps, resolution);

    // Trace 1: 葉温 (Leaf Temp)
    const traceLeaf: any = {
      x: xValues,
      y: leafTemps,
      name: '葉温 (℃)',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#10b981', width: 2 },
      yaxis: 'y1',
      hovertemplate: '%{x|%m/%d %H:%M}<br>葉温: %{y:.1f} ℃<extra></extra>',
    };

    // Trace 2: 気温 (Air Temp)
    const traceAir: any = {
      x: xValues,
      y: airTemps,
      name: '気温 (℃)',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#38bdf8', width: 1.5, dash: 'dot' },
      yaxis: 'y1',
      hovertemplate: '%{x|%m/%d %H:%M}<br>気温: %{y:.1f} ℃<extra></extra>',
    };

    // Trace 3: 葉温-気温差 (ΔT)
    const traceDeltaT: any = {
      x: xValues,
      y: deltaTs,
      name: '葉気温差 ΔT (℃)',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#f59e0b', width: 1.5 },
      yaxis: 'y2',
      hovertemplate: '%{x|%m/%d %H:%M}<br>ΔT: %{y:+.2f} ℃<extra></extra>',
    };

    // Trace 4: 根圏土壌水分 (Soil Raw)
    const traceSoil: any = {
      x: xValues,
      y: soils,
      name: '土壌水分 Raw',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#a855f7', width: 2 },
      fill: 'tozeroy',
      fillcolor: 'rgba(168, 85, 247, 0.08)',
      yaxis: 'y3',
      hovertemplate: '%{x|%m/%d %H:%M}<br>土壌Raw: %{y}<extra></extra>',
    };

    // Trace 5: 生体再構成損失 (AI Loss)
    const traceLoss: any = {
      x: xValues,
      y: losses,
      name: 'AI 再構成損失 (Loss)',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#06b6d4', width: 1.5 },
      yaxis: 'y4',
      hovertemplate: '%{x|%m/%d %H:%M}<br>Loss: %{y:.4f}<extra></extra>',
    };

    // Trace 6: ストレススコア (Stress Score)
    const traceStress: any = {
      x: xValues,
      y: stresses,
      name: '総合ストレス (0-100)',
      type: 'scatter',
      mode: 'lines',
      line: { color: '#ef4444', width: 1.5 },
      yaxis: 'y4',
      hovertemplate: '%{x|%m/%d %H:%M}<br>ストレス: %{y}<extra></extra>',
    };

    const data = [traceLeaf, traceAir, traceDeltaT, traceSoil, traceLoss, traceStress];

    const layout: any = {
      paper_bgcolor: 'transparent',
      plot_bgcolor: 'rgba(15, 23, 42, 0.4)',
      margin: { l: 45, r: 40, t: 25, b: 35 },
      showlegend: true,
      legend: {
        orientation: 'h',
        x: 0,
        y: 1.15,
        font: { color: '#94a3b8', size: 10 },
      },
      grid: {
        rows: 3,
        columns: 1,
        pattern: 'independent',
        roworder: 'top to bottom',
      },
      xaxis: {
        type: 'date',
        tickvals: tickvals.length > 0 ? tickvals : undefined,
        ticktext: ticktext.length > 0 ? ticktext : undefined,
        tickcolor: '#334155',
        tickfont: { color: '#94a3b8', size: 10 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        zerolinecolor: 'rgba(51, 65, 85, 0.6)',
      },
      xaxis2: {
        type: 'date',
        tickvals: tickvals.length > 0 ? tickvals : undefined,
        ticktext: ticktext.length > 0 ? ticktext : undefined,
        tickfont: { color: '#94a3b8', size: 10 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        zerolinecolor: 'rgba(51, 65, 85, 0.6)',
      },
      xaxis3: {
        type: 'date',
        tickvals: tickvals.length > 0 ? tickvals : undefined,
        ticktext: ticktext.length > 0 ? ticktext : undefined,
        tickfont: { color: '#94a3b8', size: 10 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        zerolinecolor: 'rgba(51, 65, 85, 0.6)',
      },
      yaxis: {
        title: { text: '温度 (℃)', font: { size: 10, color: '#94a3b8' } },
        tickfont: { color: '#94a3b8', size: 9 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        domain: [0.72, 1.0],
      },
      yaxis2: {
        title: { text: 'ΔT (℃)', font: { size: 10, color: '#f59e0b' } },
        tickfont: { color: '#f59e0b', size: 9 },
        overlaying: 'y',
        side: 'right',
        domain: [0.72, 1.0],
        showgrid: false,
      },
      yaxis3: {
        title: { text: '土壌 Raw', font: { size: 10, color: '#a855f7' } },
        tickfont: { color: '#a855f7', size: 9 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        domain: [0.38, 0.64],
        autorange: 'reversed',
      },
      yaxis4: {
        title: { text: 'AI Loss / 総合ストレス', font: { size: 10, color: '#06b6d4' } },
        tickfont: { color: '#06b6d4', size: 9 },
        gridcolor: 'rgba(51, 65, 85, 0.4)',
        domain: [0.0, 0.28],
      },
    };

    const config: any = {
      responsive: true,
      displayModeBar: false,
    };

    Plotly.purge(chartContainerRef.current);
    Plotly.newPlot(chartContainerRef.current, data, layout, config);

    const handleResize = () => {
      if (chartContainerRef.current) {
        Plotly.Plots.resize(chartContainerRef.current);
      }
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      if (chartContainerRef.current) {
        Plotly.purge(chartContainerRef.current);
      }
    };
  }, [filteredRecords, resolution]);

  return (
    <div className="w-full space-y-4">
      {/* 1. Header Toolbar (Resolution Selector, Views, Actions) */}
      <div className="flex flex-wrap items-center justify-between gap-3 bg-slate-950/60 border border-slate-800/80 p-4 rounded-3xl backdrop-blur-md shadow-xl">
        {/* Left: Title & Mode Toggle */}
        <div className="flex items-center space-x-3">
          <div className="p-2.5 rounded-2xl bg-teal-500/10 border border-teal-500/30 text-teal-400">
            <LineChart className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h2 className="text-sm font-black tracking-tight text-white uppercase">
                時系列推移 ＆ 履歴分析
              </h2>
              <span className="text-[10px] font-mono text-slate-400 bg-slate-900 px-2 py-0.5 rounded border border-slate-800">
                10分蓄積サンプリング
              </span>
            </div>
            <p className="text-[11px] text-slate-400">
              全 {records.length} 件蓄積 &bull; 期間内 {filteredRecords.length} 件
            </p>
          </div>
        </div>

        {/* Middle: Resolution Selector (時間・日・週・月・カレンダー指定) */}
        <div className="flex items-center bg-slate-900/90 border border-slate-800 rounded-2xl p-1 gap-1 text-xs">
          <button
            onClick={() => onChangeResolution('1h')}
            className={`px-3 py-1.5 rounded-xl font-bold transition cursor-pointer ${
              resolution === '1h'
                ? 'bg-teal-500 text-slate-950 shadow-md'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            時間 (1h)
          </button>
          <button
            onClick={() => onChangeResolution('24h')}
            className={`px-3 py-1.5 rounded-xl font-bold transition cursor-pointer ${
              resolution === '24h'
                ? 'bg-teal-500 text-slate-950 shadow-md'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            日 (24h)
          </button>
          <button
            onClick={() => onChangeResolution('7d')}
            className={`px-3 py-1.5 rounded-xl font-bold transition cursor-pointer ${
              resolution === '7d'
                ? 'bg-teal-500 text-slate-950 shadow-md'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            週 (7d)
          </button>
          <button
            onClick={() => onChangeResolution('30d')}
            className={`px-3 py-1.5 rounded-xl font-bold transition cursor-pointer ${
              resolution === '30d'
                ? 'bg-teal-500 text-slate-950 shadow-md'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            月 (30d)
          </button>
          <button
            onClick={() => setShowDateModal(true)}
            className={`flex items-center gap-1 px-3 py-1.5 rounded-xl font-bold transition cursor-pointer ${
              resolution === 'custom'
                ? 'bg-teal-500 text-slate-950 shadow-md'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            <Calendar className="w-3.5 h-3.5" />
            <span>期間指定</span>
          </button>
        </div>

        {/* View Layout Switcher & Action Buttons */}
        <div className="flex items-center gap-2">
          {/* View Mode */}
          <div className="flex items-center bg-slate-900/90 border border-slate-800 rounded-2xl p-1 text-xs">
            <button
              onClick={() => setViewMode('split')}
              className={`p-1.5 rounded-xl transition cursor-pointer ${
                viewMode === 'split' ? 'bg-slate-800 text-teal-400' : 'text-slate-400 hover:text-white'
              }`}
              title="グラフ・一覧の分割表示"
            >
              <Columns className="w-4 h-4" />
            </button>
            <button
              onClick={() => setViewMode('chart')}
              className={`p-1.5 rounded-xl transition cursor-pointer ${
                viewMode === 'chart' ? 'bg-slate-800 text-teal-400' : 'text-slate-400 hover:text-white'
              }`}
              title="グラフ最大化表示"
            >
              <LineChart className="w-4 h-4" />
            </button>
            <button
              onClick={() => setViewMode('table')}
              className={`p-1.5 rounded-xl transition cursor-pointer ${
                viewMode === 'table' ? 'bg-slate-800 text-teal-400' : 'text-slate-400 hover:text-white'
              }`}
              title="テーブル一覧表示"
            >
              <TableIcon className="w-4 h-4" />
            </button>
          </div>

          {/* Refresh */}
          <button
            onClick={onRefresh}
            disabled={isLoading}
            className="p-2 rounded-2xl bg-slate-900 border border-slate-800 text-slate-300 hover:text-white transition active:scale-95 cursor-pointer"
            title="最新履歴を再取得"
          >
            <RotateCw className={`w-4 h-4 ${isLoading ? 'animate-spin text-teal-400' : ''}`} />
          </button>

          {/* Export CSV */}
          <button
            onClick={onExportCsv}
            className="flex items-center space-x-1 px-3 py-2 rounded-2xl bg-slate-900 border border-slate-800 text-xs text-slate-300 hover:text-white hover:border-slate-700 transition active:scale-95 cursor-pointer"
            title="CSV形式でエクスポート"
          >
            <Download className="w-3.5 h-3.5" />
            <span className="hidden sm:inline">CSV</span>
          </button>

          {/* Export Parquet */}
          <button
            onClick={onExportParquet}
            className="flex items-center space-x-1 px-3 py-2 rounded-2xl bg-teal-500/10 border border-teal-500/30 text-xs text-teal-300 hover:bg-teal-500/20 transition active:scale-95 cursor-pointer"
            title="AI学習用 Parquet形式でダウンロード"
          >
            <FileCode2 className="w-3.5 h-3.5 text-teal-400" />
            <span className="hidden sm:inline">Parquet</span>
          </button>

          {/* Clear Logs */}
          <button
            onClick={onClear}
            className="p-2 rounded-2xl bg-rose-500/10 border border-rose-500/30 text-rose-400 hover:bg-rose-500/20 transition active:scale-95 cursor-pointer"
            title="サーバ上の履歴ログを全消去"
          >
            <Trash2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* 2. Statistical Highlights Banner & Last Watering Banner */}
      <div className="grid grid-cols-2 sm:grid-cols-5 gap-3">
        {/* Metric 1: Avg Leaf & Diff */}
        <div className="p-3.5 rounded-3xl bg-slate-950/40 border border-slate-800/60 backdrop-blur-md flex items-center space-x-3">
          <div className="p-2 rounded-2xl bg-emerald-500/10 text-emerald-400">
            <Leaf className="w-4 h-4" />
          </div>
          <div>
            <div className="text-[10px] text-slate-400 font-semibold">平均葉温 / 葉気温差</div>
            <div className="text-base font-black text-white font-mono">
              {stats.avgLeaf}&deg;C
              <span className="text-xs text-amber-400 font-normal ml-1">
                ({stats.avgDiff >= 0 ? '+' : ''}
                {stats.avgDiff}&deg;C)
              </span>
            </div>
          </div>
        </div>

        {/* Metric 2: Avg Air */}
        <div className="p-3.5 rounded-3xl bg-slate-950/40 border border-slate-800/60 backdrop-blur-md flex items-center space-x-3">
          <div className="p-2 rounded-2xl bg-sky-500/10 text-sky-400">
            <Thermometer className="w-4 h-4" />
          </div>
          <div>
            <div className="text-[10px] text-slate-400 font-semibold">平均環境気温</div>
            <div className="text-base font-black text-white font-mono">{stats.avgAir}&deg;C</div>
          </div>
        </div>

        {/* Metric 3: Soil Range */}
        <div className="p-3.5 rounded-3xl bg-slate-950/40 border border-slate-800/60 backdrop-blur-md flex items-center space-x-3">
          <div className="p-2 rounded-2xl bg-purple-500/10 text-purple-400">
            <Droplets className="w-4 h-4" />
          </div>
          <div>
            <div className="text-[10px] text-slate-400 font-semibold">土壌水分 (最新/最小)</div>
            <div className="text-base font-black text-white font-mono">
              {stats.curSoil}
              <span className="text-xs text-slate-400 font-normal ml-1">
                / {stats.minSoil}
              </span>
            </div>
          </div>
        </div>

        {/* Metric 4: AI Loss */}
        <div className="p-3.5 rounded-3xl bg-slate-950/40 border border-slate-800/60 backdrop-blur-md flex items-center space-x-3">
          <div className="p-2 rounded-2xl bg-teal-500/10 text-teal-400">
            <Activity className="w-4 h-4" />
          </div>
          <div>
            <div className="text-[10px] text-slate-400 font-semibold">平均再構成損失</div>
            <div className="text-base font-black text-teal-400 font-mono">{stats.avgLoss}</div>
          </div>
        </div>

        {/* Metric 5: Last Watering Status */}
        <div className="p-3.5 rounded-3xl bg-slate-950/40 border border-slate-800/60 backdrop-blur-md flex items-center space-x-3 col-span-2 sm:col-span-1">
          <div className="p-2 rounded-2xl bg-amber-500/10 text-amber-400">
            <Clock className="w-4 h-4" />
          </div>
          <div>
            <div className="text-[10px] text-slate-400 font-semibold">直近給水実績</div>
            {wateringLogs.length > 0 ? (
              <div className="text-xs font-bold text-white flex items-center gap-1 font-mono">
                <span>{wateringLogs[0].durationSec}秒</span>
                {wateringLogs[0].success ? (
                  <span className="text-emerald-400 flex items-center text-[11px]">
                    <CheckCircle2 className="w-3 h-3 mr-0.5" /> 成功
                  </span>
                ) : (
                  <span className="text-rose-400 flex items-center text-[11px]">
                    <XCircle className="w-3 h-3 mr-0.5" /> 失敗
                  </span>
                )}
              </div>
            ) : (
              <div className="text-xs text-slate-500 font-mono">給水記録なし</div>
            )}
          </div>
        </div>
      </div>

      {/* 3. Main Body: Split View / Chart Only / Table Only */}
      <div className="grid grid-cols-1 gap-4">
        {/* Chart Section */}
        {(viewMode === 'split' || viewMode === 'chart') && (
          <div className="bg-slate-950/40 border border-slate-800/60 rounded-3xl p-5 backdrop-blur-md shadow-2xl relative">
            <div className="flex items-center justify-between mb-2">
              <div className="text-xs font-bold text-slate-300 flex items-center gap-2">
                <span className="w-2 h-2 rounded-full bg-teal-400 animate-pulse" />
                <span>生体・微気象・根圏・AIマルチ推移プロット</span>
              </div>
              <span className="text-[10px] font-mono text-slate-500">
                プロット解像度: {resolution.toUpperCase()}
              </span>
            </div>

            {/* Plotly Canvas Container */}
            <div
              ref={chartContainerRef}
              className={`w-full ${viewMode === 'chart' ? 'h-[650px]' : 'h-[460px]'}`}
            />
          </div>
        )}

        {/* Table Section */}
        {(viewMode === 'split' || viewMode === 'table') && (
          <div className="bg-slate-950/40 border border-slate-800/60 rounded-3xl p-5 backdrop-blur-md shadow-2xl">
            {/* Table Header Filter & Pagination Controls */}
            <div className="flex flex-wrap items-center justify-between gap-3 mb-4 pb-3 border-b border-slate-800/80">
              <div className="flex items-center space-x-3">
                <TableIcon className="w-4 h-4 text-teal-400" />
                <h3 className="text-xs font-bold uppercase tracking-wider text-slate-300">
                  サンプリング履歴ログ一覧
                </h3>
                <span className="text-[10px] text-slate-500 font-mono">
                  ({tableRecords.length} 件該当)
                </span>
              </div>

              <div className="flex flex-wrap items-center gap-3">
                {/* Status Filter */}
                <div className="flex items-center space-x-1.5 text-xs">
                  <span className="text-slate-400 text-[11px]">状態:</span>
                  <select
                    value={statusFilter}
                    onChange={(e) => {
                      setStatusFilter(e.target.value);
                      setPage(1);
                    }}
                    className="bg-slate-900 border border-slate-800 text-slate-200 text-xs rounded-xl px-2.5 py-1 outline-none focus:border-teal-500 cursor-pointer"
                  >
                    <option value="all">すべて</option>
                    <option value="HEALTHY">HEALTHY (平常)</option>
                    <option value="DRY_STRESS">DRY_STRESS (乾燥)</option>
                    <option value="HEAT_STRESS">HEAT_STRESS (熱)</option>
                    <option value="WATERING">WATERING (給水)</option>
                  </select>
                </div>

                {/* Page Size */}
                <div className="flex items-center space-x-1.5 text-xs">
                  <span className="text-slate-400 text-[11px]">件数:</span>
                  <select
                    value={pageSize}
                    onChange={(e) => {
                      setPageSize(Number(e.target.value));
                      setPage(1);
                    }}
                    className="bg-slate-900 border border-slate-800 text-slate-200 text-xs rounded-xl px-2 py-1 outline-none focus:border-teal-500 cursor-pointer"
                  >
                    <option value={10}>10件</option>
                    <option value={15}>15件</option>
                    <option value={30}>30件</option>
                    <option value={50}>50件</option>
                  </select>
                </div>

                {/* Pagination */}
                <div className="flex items-center space-x-1 text-xs font-mono">
                  <button
                    onClick={() => setPage((p) => Math.max(1, p - 1))}
                    disabled={page === 1}
                    className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800 text-slate-300 disabled:opacity-40 disabled:cursor-not-allowed hover:bg-slate-800"
                  >
                    &lt;
                  </button>
                  <span className="px-2 text-slate-400">
                    {page} / {totalPages}
                  </span>
                  <button
                    onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
                    disabled={page === totalPages}
                    className="px-2 py-1 rounded-lg bg-slate-900 border border-slate-800 text-slate-300 disabled:opacity-40 disabled:cursor-not-allowed hover:bg-slate-800"
                  >
                    &gt;
                  </button>
                </div>
              </div>
            </div>

            {/* Scrollable Data Table */}
            <div className="overflow-x-auto">
              <table className="w-full text-left border-collapse text-xs font-mono">
                <thead>
                  <tr className="border-b border-slate-800/80 text-[11px] text-slate-400">
                    <th
                      onClick={() => setSortOrder((prev) => (prev === 'desc' ? 'asc' : 'desc'))}
                      className="py-2.5 px-3 cursor-pointer hover:text-white select-none transition"
                      title="クリックで日付の降順・昇順を切り替え"
                    >
                      <div className="flex items-center gap-1.5">
                        <span>日時 (JST)</span>
                        <span className="text-[10px] font-bold text-teal-400 bg-teal-950/80 px-1 py-0.2 rounded border border-teal-800/50">
                          {sortOrder === 'desc' ? '▼ 降順' : '▲ 昇順'}
                        </span>
                      </div>
                    </th>
                    <th className="py-2.5 px-3">状態</th>
                    <th className="py-2.5 px-3">ストレス</th>
                    <th className="py-2.5 px-3">葉温 (℃)</th>
                    <th className="py-2.5 px-3">気温 (℃)</th>
                    <th className="py-2.5 px-3">ΔT (℃)</th>
                    <th className="py-2.5 px-3">湿度 (%)</th>
                    <th className="py-2.5 px-3">土壌Raw</th>
                    <th className="py-2.5 px-3">照度 (lx)</th>
                    <th className="py-2.5 px-3">AI Loss</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/40">
                  {pagedRecords.length === 0 ? (
                    <tr>
                      <td colSpan={10} className="py-8 text-center text-slate-500 font-sans">
                        該当する履歴データがありません
                      </td>
                    </tr>
                  ) : (
                    pagedRecords.map((r) => {
                      const dt = new Date(r.timestamp * 1000);
                      const timeStr = `${dt.getFullYear()}/${(dt.getMonth() + 1)
                        .toString()
                        .padStart(2, '0')}/${dt.getDate().toString().padStart(2, '0')} ${dt
                        .getHours()
                        .toString()
                        .padStart(2, '0')}:${dt.getMinutes().toString().padStart(2, '0')}`;

                      return (
                        <tr key={r.timestamp} className="hover:bg-slate-900/50 transition">
                          <td className="py-2 px-3 text-slate-300 whitespace-nowrap">{timeStr}</td>
                          <td className="py-2 px-3 whitespace-nowrap">
                            <span
                              className={`px-2 py-0.5 rounded-full text-[10px] font-bold ${
                                r.status === 'HEALTHY'
                                  ? 'bg-emerald-950/80 text-emerald-400 border border-emerald-800/50'
                                  : r.status === 'DRY_STRESS'
                                  ? 'bg-amber-950/80 text-amber-400 border border-amber-800/50'
                                  : r.status === 'HEAT_STRESS'
                                  ? 'bg-rose-950/80 text-rose-400 border border-rose-800/50'
                                  : 'bg-teal-950/80 text-teal-400 border border-teal-800/50'
                              }`}
                            >
                              {r.status}
                            </span>
                          </td>
                          <td className="py-2 px-3 font-bold text-white">{r.stress ?? '--'}</td>
                          <td className="py-2 px-3 text-emerald-400 font-bold">
                            {r.leaf_temp?.toFixed(1) ?? '--'}
                          </td>
                          <td className="py-2 px-3 text-sky-400">
                            {r.air_temp?.toFixed(1) ?? '--'}
                          </td>
                          <td
                            className={`py-2 px-3 font-semibold ${
                              (r.leaf_air_diff ?? 0) < 0 ? 'text-emerald-400' : 'text-amber-400'
                            }`}
                          >
                            {(r.leaf_air_diff ?? 0) >= 0 ? '+' : ''}
                            {r.leaf_air_diff?.toFixed(2) ?? '--'}
                          </td>
                          <td className="py-2 px-3 text-indigo-300">
                            {r.humidity?.toFixed(0) ?? '--'}%
                          </td>
                          <td className="py-2 px-3 text-purple-400 font-bold">{r.soil_raw}</td>
                          <td className="py-2 px-3 text-amber-300">{r.lux}</td>
                          <td className="py-2 px-3 text-teal-300">
                            {r.ai_loss?.toFixed(4) ?? '--'}
                          </td>
                        </tr>
                      );
                    })
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}
      </div>

      {/* 4. Calendar Custom Range Modal (何日から何日の何時まで ＆ 1日単位チェックボックス) */}
      {showDateModal && (
        <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-3xl w-full max-w-md p-6 shadow-2xl space-y-5">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div className="flex items-center gap-2">
                <Calendar className="w-5 h-5 text-teal-400" />
                <h3 className="font-bold text-white text-base">集計期間の指定</h3>
              </div>
              <button
                onClick={() => setShowDateModal(false)}
                className="text-slate-400 hover:text-white px-2 py-1 rounded-lg text-sm cursor-pointer"
              >
                ✕
              </button>
            </div>

            {/* Checkbox: 1日単位（開始日のみ） */}
            <div className="flex items-center space-x-2 bg-slate-950 p-3 rounded-2xl border border-slate-800">
              <input
                type="checkbox"
                id="singleDayOnly"
                checked={customRange.singleDayOnly ?? false}
                onChange={(e) =>
                  setCustomRange((prev) => ({ ...prev, singleDayOnly: e.target.checked }))
                }
                className="w-4 h-4 text-teal-500 rounded focus:ring-teal-400 cursor-pointer"
              />
              <label htmlFor="singleDayOnly" className="text-xs text-slate-200 cursor-pointer font-medium">
                開始日のみ（1日単位で表示: 00:00 〜 23:59）
              </label>
            </div>

            {/* Start Date & Hour */}
            <div className="space-y-1.5">
              <label className="text-xs font-semibold text-slate-300">開始日時</label>
              <div className="grid grid-cols-2 gap-2">
                <input
                  type="date"
                  value={customRange.startDate}
                  onChange={(e) =>
                    setCustomRange((prev) => ({ ...prev, startDate: e.target.value }))
                  }
                  className="bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-xs text-white outline-none focus:border-teal-400"
                />
                <select
                  value={customRange.startHour}
                  disabled={customRange.singleDayOnly}
                  onChange={(e) =>
                    setCustomRange((prev) => ({ ...prev, startHour: Number(e.target.value) }))
                  }
                  className="bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-xs text-white outline-none focus:border-teal-400 disabled:opacity-40"
                >
                  {Array.from({ length: 24 }).map((_, i) => (
                    <option key={i} value={i}>
                      {i} 時
                    </option>
                  ))}
                </select>
              </div>
            </div>

            {/* End Date & Hour */}
            {!customRange.singleDayOnly && (
              <div className="space-y-1.5">
                <label className="text-xs font-semibold text-slate-300">終了日時</label>
                <div className="grid grid-cols-2 gap-2">
                  <input
                    type="date"
                    value={customRange.endDate}
                    onChange={(e) =>
                      setCustomRange((prev) => ({ ...prev, endDate: e.target.value }))
                    }
                    className="bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-xs text-white outline-none focus:border-teal-400"
                  />
                  <select
                    value={customRange.endHour}
                    onChange={(e) =>
                      setCustomRange((prev) => ({ ...prev, endHour: Number(e.target.value) }))
                    }
                    className="bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-xs text-white outline-none focus:border-teal-400"
                  >
                    {Array.from({ length: 24 }).map((_, i) => (
                      <option key={i} value={i}>
                        {i} 時
                      </option>
                    ))}
                  </select>
                </div>
              </div>
            )}

            {/* Footer Buttons */}
            <div className="flex items-center justify-end space-x-2 pt-3 border-t border-slate-800">
              <button
                onClick={() => setShowDateModal(false)}
                className="px-4 py-2 rounded-xl bg-slate-800 text-slate-300 hover:bg-slate-700 text-xs font-semibold cursor-pointer"
              >
                キャンセル
              </button>
              <button
                onClick={() => {
                  onChangeResolution('custom');
                  setShowDateModal(false);
                }}
                className="px-4 py-2 rounded-xl bg-gradient-to-r from-teal-500 to-emerald-400 text-slate-950 text-xs font-black hover:opacity-90 transition shadow-lg cursor-pointer"
              >
                この期間で表示
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
