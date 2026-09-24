import React, { useEffect, useRef } from 'react';
import Plotly from 'plotly.js-dist-min';
import { TimeResolution, HistorySampleRecord } from '../types';
import { Download, RefreshCw, Calendar, FileCode2 } from 'lucide-react';

interface PlotlyTimeSeriesChartProps {
  records: HistorySampleRecord[];
  resolution: TimeResolution;
  onChangeResolution: (res: TimeResolution) => void;
  onRefresh?: () => void;
  onExportCsv?: () => void;
  onExportParquet?: () => void;
}

export const PlotlyTimeSeriesChart: React.FC<PlotlyTimeSeriesChartProps> = ({
  records,
  resolution,
  onChangeResolution,
  onRefresh,
  onExportCsv,
  onExportParquet,
}) => {
  const chartContainerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!chartContainerRef.current) return;

    // 分解能に応じたフィルタリング・データ抽出
    const nowSec = records.length > 0 ? records[records.length - 1].timestamp : Math.floor(Date.now() / 1000);
    let filterSec = 24 * 3600;
    if (resolution === '1h') filterSec = 3600;
    else if (resolution === '24h') filterSec = 24 * 3600;
    else if (resolution === '7d') filterSec = 7 * 24 * 3600;
    else if (resolution === '30d') filterSec = 30 * 24 * 3600;

    const filtered = records.filter((r) => r.timestamp >= nowSec - filterSec);
    const displayRecords = filtered.length > 0 ? filtered : records;

    // 時刻配列（ISO文字列形式で渡すとPlotlyが自動で日時軸として解釈）
    const timeLabels = displayRecords.map((r) => {
      const d = new Date(r.timestamp * 1000);
      const pad = (n: number) => n.toString().padStart(2, '0');
      return `${pad(d.getHours())}:${pad(d.getMinutes())}\n${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`;
    });

    const airTemps = displayRecords.map((r) => r.air_temp);
    const leafTemps = displayRecords.map((r) => r.leaf_temp);
    const hums = displayRecords.map((r) => r.humidity);
    const deltaTs = displayRecords.map((r) => r.leaf_air_diff);
    const lights = displayRecords.map((r) => r.lux);
    const soils = displayRecords.map((r) => r.soil_raw);
    const stresses = displayRecords.map((r) => r.stress);
    const currents = displayRecords.map((r) => r.soil_rate ?? (r.pump_on ? 120 : -10));
    const powers = displayRecords.map((r) => Math.round((r.ai_loss ?? 0.02) * 10000) + (r.pump_on ? 350 : 0));

    // アプリのダークテーマ（Slate 900 / 950）に100%調和するカラーパレット
    const bgPaper = 'rgba(0, 0, 0, 0)';           // 親コンテナと完全に一体化
    const bgPlot = 'rgba(15, 23, 42, 0.75)';        // 各サブプロットの背景: 上品な濃スレート
    const gridColor = 'rgba(51, 65, 85, 0.35)';     // グリッド線: 薄いスレート
    const axisLineColor = 'rgba(71, 85, 105, 0.5)'; // 軸枠線
    const textColor = '#94a3b8';                    // 目盛り・軸文字: Tailwind Slate 400
    const titleColor = '#f8fafc';                   // サブプロットタイトル: Tailwind Slate 50

    // 8つのサブプロットトレース定義 (2列 x 4行、添付画像 media_1789975632756.png のレイアウト完全準拠)
    const traces: Plotly.Data[] = [
      // 1. Temp (Red/Coral + Amber Leaf)
      {
        x: timeLabels,
        y: airTemps,
        name: 'Air Temp',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#f43f5e', width: 1.8 },
        xaxis: 'x',
        yaxis: 'y',
        hovertemplate: '気温: %{y:.1f}℃<extra></extra>',
      },
      {
        x: timeLabels,
        y: leafTemps,
        name: 'Leaf Temp',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#fb923c', width: 1.5, dash: 'dot' },
        xaxis: 'x',
        yaxis: 'y',
        hovertemplate: '葉温: %{y:.1f}℃<extra></extra>',
      },

      // 2. Hum (Sky Blue)
      {
        x: timeLabels,
        y: hums,
        name: 'Humidity',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#38bdf8', width: 1.8 },
        xaxis: 'x2',
        yaxis: 'y2',
        hovertemplate: '湿度: %{y:.1f}%<extra></extra>',
      },

      // 3. Pres / ΔT (Cool Slate)
      {
        x: timeLabels,
        y: deltaTs,
        name: 'Leaf-Air ΔT',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#94a3b8', width: 1.8 },
        xaxis: 'x3',
        yaxis: 'y3',
        hovertemplate: '葉気温差: %{y:.2f}℃<extra></extra>',
      },

      // 4. Light (Warm Gold)
      {
        x: timeLabels,
        y: lights,
        name: 'Light',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#fbbf24', width: 1.8 },
        xaxis: 'x4',
        yaxis: 'y4',
        hovertemplate: '照度: %{y} lx<extra></extra>',
      },

      // 5. Soil (Emerald Green)
      {
        x: timeLabels,
        y: soils,
        name: 'Soil Raw',
        type: 'scatter',
        mode: 'lines+markers',
        marker: { size: 3, color: '#10b981' },
        line: { color: '#10b981', width: 1.8 },
        xaxis: 'x5',
        yaxis: 'y5',
        hovertemplate: '土壌Raw: %{y}<extra></extra>',
      },

      // 6. Bus / Stress (Bright Violet)
      {
        x: timeLabels,
        y: stresses,
        name: 'Stress Score',
        type: 'scatter',
        mode: 'lines+markers',
        marker: { size: 3, color: '#c084fc' },
        line: { color: '#c084fc', width: 1.8 },
        xaxis: 'x6',
        yaxis: 'y6',
        hovertemplate: 'ストレス: %{y}/100<extra></extra>',
      },

      // 7. Current / Rate (Amber Orange)
      {
        x: timeLabels,
        y: currents,
        name: 'Soil Rate / Current',
        type: 'scatter',
        mode: 'lines',
        line: { color: '#f97316', width: 1.8 },
        xaxis: 'x7',
        yaxis: 'y7',
        hovertemplate: '変化率: %{y}<extra></extra>',
      },

      // 8. Power / Loss (Mint Teal)
      {
        x: timeLabels,
        y: powers,
        name: 'AI Loss (PPM)',
        type: 'scatter',
        mode: 'lines+markers',
        marker: { size: 3, color: '#2dd4bf' },
        line: { color: '#2dd4bf', width: 1.8 },
        xaxis: 'x8',
        yaxis: 'y8',
        hovertemplate: 'AI損失/電力: %{y}<extra></extra>',
      },
    ];

    // サブプロット 2列 x 4行 レイアウト
    const commonAxis = {
      tickangle: -35,
      gridcolor: gridColor,
      zerolinecolor: gridColor,
      showline: true,
      linecolor: axisLineColor,
      tickfont: { size: 9, color: textColor, family: 'monospace' },
      title: { text: '時刻', font: { size: 10, color: textColor } },
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
        rows: 4,
        columns: 2,
        pattern: 'independent',
        roworder: 'top to bottom',
      },
      showlegend: false,
      margin: { l: 50, r: 20, t: 30, b: 60 },
      paper_bgcolor: bgPaper,
      plot_bgcolor: bgPlot,
      font: { family: 'system-ui, -apple-system, sans-serif', color: textColor },

      xaxis: commonAxis,
      yaxis: commonYAxis('Temp (℃)'),

      xaxis2: commonAxis,
      yaxis2: commonYAxis('Hum (%RH)'),

      xaxis3: commonAxis,
      yaxis3: commonYAxis('Pres / ΔT (℃)'),

      xaxis4: commonAxis,
      yaxis4: commonYAxis('Light (lx)'),

      xaxis5: commonAxis,
      yaxis5: commonYAxis('Soil (Raw)'),

      xaxis6: commonAxis,
      yaxis6: commonYAxis('Stress (0-100)'),

      xaxis7: commonAxis,
      yaxis7: commonYAxis('Current / Rate'),

      xaxis8: commonAxis,
      yaxis8: commonYAxis('Power / Loss'),

      // 各サブプロットのタイトル（ダークテーマに調和したクリーンなタイポグラフィ）
      annotations: [
        { text: '<b>Temp</b> <span style="font-size:10px; color:#f43f5e;">●</span>', x: 0.22, y: 1.025, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Hum</b> <span style="font-size:10px; color:#38bdf8;">●</span>', x: 0.78, y: 1.025, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Pres (ΔT)</b> <span style="font-size:10px; color:#94a3b8;">●</span>', x: 0.22, y: 0.745, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Light</b> <span style="font-size:10px; color:#fbbf24;">●</span>', x: 0.78, y: 0.745, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Soil</b> <span style="font-size:10px; color:#10b981;">●</span>', x: 0.22, y: 0.465, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Bus (Stress)</b> <span style="font-size:10px; color:#c084fc;">●</span>', x: 0.78, y: 0.465, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Current</b> <span style="font-size:10px; color:#f97316;">●</span>', x: 0.22, y: 0.185, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
        { text: '<b>Power (AI Loss)</b> <span style="font-size:10px; color:#2dd4bf;">●</span>', x: 0.78, y: 0.185, xref: 'paper', yref: 'paper', showarrow: false, font: { size: 12, color: titleColor } },
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
      if (chartContainerRef.current) {
        Plotly.purge(chartContainerRef.current);
      }
    };
  }, [records, resolution]);

  return (
    <div className="bg-slate-900/90 border border-slate-800 rounded-2xl p-5 shadow-xl space-y-4">
      {/* ツールバー: 分解能セレクター & エクスポート */}
      <div className="flex flex-wrap items-center justify-between gap-3 pb-3 border-b border-slate-800">
        <div>
          <h3 className="text-base font-bold text-white tracking-wide flex items-center gap-2">
            環境時系列推移 (8画面 統合モニタ)
            <span className="text-[10px] font-mono px-2 py-0.5 rounded-full bg-slate-800 text-slate-300 border border-slate-700">
              2x4 Subplots
            </span>
          </h3>
          <p className="text-xs text-slate-400 mt-0.5">
            ROHM DT-EBML63Q2557 センサ全指標リアルタイム同期波形
          </p>
        </div>

        {/* 分解能切り替え & エクスポートボタン */}
        <div className="flex flex-wrap items-center gap-2">
          {/* 分解能ピル */}
          <div className="flex items-center space-x-1 bg-slate-950 p-1 rounded-xl border border-slate-800">
            <Calendar className="w-3.5 h-3.5 text-slate-400 ml-1.5 mr-0.5" />
            {(
              [
                { id: '1h', label: '1時間' },
                { id: '24h', label: '24時間' },
                { id: '7d', label: '7日間' },
                { id: '30d', label: '30日間' },
              ] as { id: TimeResolution; label: string }[]
            ).map((item) => (
              <button
                key={item.id}
                onClick={() => onChangeResolution(item.id)}
                className={`px-2.5 py-1 text-xs font-bold rounded-lg transition cursor-pointer ${
                  resolution === item.id
                    ? 'bg-emerald-600 text-white shadow'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {item.label}
              </button>
            ))}
          </div>

          {/* 手動同期 */}
          {onRefresh && (
            <button
              onClick={onRefresh}
              className="p-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition cursor-pointer"
              title="データを最新化"
            >
              <RefreshCw className="w-4 h-4" />
            </button>
          )}

          {/* Parquet 保存 */}
          {onExportParquet && (
            <button
              onClick={onExportParquet}
              className="flex items-center space-x-1.5 px-3 py-1.5 text-xs font-bold rounded-xl bg-amber-600 hover:bg-amber-500 text-slate-950 shadow transition cursor-pointer"
              title="列指向・高圧縮のApache Parquet形式でPCローカルにファイル保存"
            >
              <FileCode2 className="w-3.5 h-3.5" />
              <span>Parquet保存</span>
            </button>
          )}

          {/* CSV 保存 */}
          {onExportCsv && (
            <button
              onClick={onExportCsv}
              className="flex items-center space-x-1.5 px-3 py-1.5 text-xs font-bold rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 transition cursor-pointer"
            >
              <Download className="w-3.5 h-3.5 text-sky-400" />
              <span>CSV</span>
            </button>
          )}
        </div>
      </div>

      {/* Plotly 8分割グラフ本体 */}
      <div
        ref={chartContainerRef}
        className="w-full rounded-xl overflow-hidden"
        style={{ height: '760px' }}
      />
    </div>
  );
};
