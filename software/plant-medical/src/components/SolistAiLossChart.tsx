import React from 'react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
} from 'chart.js';
import { Line } from 'react-chartjs-2';
import { TrendingDown, Info } from 'lucide-react';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
);

interface SolistAiLossChartProps {
  timeLabels: string[];
  lossHistory: number[];
  isHistory?: boolean;
}

export const SolistAiLossChart: React.FC<SolistAiLossChartProps> = ({
  timeLabels,
  lossHistory,
  isHistory = false,
}) => {
  const currentLoss = lossHistory.length > 0 ? lossHistory[lossHistory.length - 1] : 0;
  const minLoss = lossHistory.length > 0 ? Math.min(...lossHistory) : 0;
  const maxLoss = lossHistory.length > 0 ? Math.max(...lossHistory) : 0;
  const avgLoss =
    lossHistory.length > 0
      ? lossHistory.reduce((a, b) => a + b, 0) / lossHistory.length
      : 0;

  const ANOMALY_THRESHOLD = 0.08;

  const chartData = {
    labels: timeLabels,
    datasets: [
      {
        label: '再構成誤差 (Reconstruction Loss)',
        data: lossHistory,
        borderColor: '#10b981',
        backgroundColor: (context: any) => {
          const ctx = context.chart.ctx;
          const gradient = ctx.createLinearGradient(0, 0, 0, 200);
          gradient.addColorStop(0, 'rgba(16, 185, 129, 0.35)');
          gradient.addColorStop(1, 'rgba(16, 185, 129, 0.0)');
          return gradient;
        },
        borderWidth: 2,
        pointRadius: 2,
        pointHoverRadius: 5,
        tension: 0.3,
        fill: true,
      },
      {
        label: '異常検知しきい値 (Threshold: 0.080)',
        data: Array(timeLabels.length).fill(ANOMALY_THRESHOLD),
        borderColor: 'rgba(244, 63, 94, 0.7)',
        backgroundColor: 'transparent',
        borderDash: [6, 4],
        borderWidth: 1.5,
        pointRadius: 0,
        fill: false,
      },
    ],
  };

  const chartOptions: any = {
    responsive: true,
    maintainAspectRatio: false,
    animation: {
      duration: 300,
    },
    scales: {
      x: {
        grid: {
          color: 'rgba(51, 65, 85, 0.4)',
        },
        ticks: {
          color: '#94a3b8',
          maxTicksLimit: 8,
          font: { family: 'monospace', size: 10 },
        },
      },
      y: {
        min: 0,
        suggestedMax: Math.max(0.12, maxLoss * 1.2),
        grid: {
          color: 'rgba(51, 65, 85, 0.4)',
        },
        ticks: {
          color: '#94a3b8',
          font: { family: 'monospace', size: 10 },
          callback: (value: number) => value.toFixed(3),
        },
      },
    },
    plugins: {
      legend: {
        position: 'top' as const,
        labels: {
          color: '#cbd5e1',
          font: { size: 11 },
          boxWidth: 12,
          usePointStyle: true,
        },
      },
      tooltip: {
        mode: 'index' as const,
        intersect: false,
        backgroundColor: 'rgba(15, 23, 42, 0.95)',
        borderColor: '#334155',
        borderWidth: 1,
        titleColor: '#e2e8f0',
        bodyColor: '#cbd5e1',
        titleFont: { family: 'monospace', size: 11 },
        bodyFont: { family: 'monospace', size: 11 },
        callbacks: {
          label: (context: any) => {
            return ` ${context.dataset.label}: ${context.parsed.y.toFixed(4)}`;
          },
        },
      },
    },
  };

  return (
    <div className="bg-slate-900/90 border border-slate-700/80 rounded-2xl p-6 shadow-xl backdrop-blur-sm flex flex-col justify-between">
      {/* Header */}
      <div className="flex flex-wrap items-center justify-between gap-3 mb-4">
        <div className="flex items-center space-x-2">
          <div className="p-2 bg-emerald-500/10 border border-emerald-500/30 rounded-lg text-emerald-400">
            <TrendingDown className="w-5 h-5" />
          </div>
          <div>
            <h3 className="text-base font-bold text-white tracking-wide">
              Solist-AI™ 学習収束・再構成誤差推移 {isHistory && <span className="text-xs text-indigo-400 font-mono">(過去ログ)</span>}
            </h3>
            <p className="text-xs text-slate-400">
              {isHistory
                ? 'Autoencoder MSE Reconstruction Loss (選択期間の収束推移)'
                : 'Autoencoder MSE Reconstruction Loss (リアルタイム追従)'}
            </p>
          </div>
        </div>

        {/* Stats Pills */}
        <div className="flex items-center space-x-2 font-mono text-xs">
          <div className="px-2.5 py-1 bg-slate-800 border border-slate-700 rounded-lg text-slate-300">
            Min: <span className="text-emerald-400 font-bold">{minLoss.toFixed(4)}</span>
          </div>
          <div className="px-2.5 py-1 bg-slate-800 border border-slate-700 rounded-lg text-slate-300">
            Avg: <span className="text-cyan-400 font-bold">{avgLoss.toFixed(4)}</span>
          </div>
          <div className="px-2.5 py-1 bg-slate-800 border border-slate-700 rounded-lg text-slate-300">
            Now: <span className={`font-bold ${currentLoss > ANOMALY_THRESHOLD ? 'text-rose-400' : 'text-emerald-400'}`}>{currentLoss.toFixed(4)}</span>
          </div>
        </div>
      </div>

      {/* Chart Canvas */}
      <div className="h-56 w-full relative mb-3">
        {timeLabels.length > 0 ? (
          <Line data={chartData} options={chartOptions} />
        ) : (
          <div className="h-full flex items-center justify-center text-slate-500 text-xs font-mono">
            テレメトリデータ受信待機中...
          </div>
        )}
      </div>

      {/* Loss Interpretation Guide */}
      <div className="bg-slate-950/70 border border-slate-800 rounded-xl p-3 flex items-start space-x-2.5 text-xs text-slate-400 leading-relaxed">
        <Info className="w-4 h-4 text-cyan-400 flex-shrink-0 mt-0.5" />
        <div>
          <span className="font-semibold text-slate-200">メカニズム解説: </span>
          Solist-AI™は正常時の8次元環境・生体相関を学習しており、健全状態では再構成誤差が 0.01〜0.05 に収束します。
          水ストレスや蒸散機能障害が発生すると、既知の健全相関から乖離して誤差が急上昇（&gt; 0.080）し、しきい値判定ルールよりも早期に潜在的異変を捉えます。
        </div>
      </div>
    </div>
  );
};
