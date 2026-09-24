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
import { TrendingDown } from 'lucide-react';

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
  phase?: number;
  trainCount?: number;
  anomalyScore?: number;
  currentLoss?: number;
}

export const SolistAiLossChart: React.FC<SolistAiLossChartProps> = ({
  timeLabels,
  lossHistory,
  isHistory = false,
  phase = 2,
  trainCount = 52,
  anomalyScore = 4,
  currentLoss: propLoss,
}) => {
  const currentLoss = propLoss ?? (lossHistory.length > 0 ? lossHistory[lossHistory.length - 1] : 0.0215);
  const minLoss = lossHistory.length > 0 ? Math.min(...lossHistory) : 0.018;
  const maxLoss = lossHistory.length > 0 ? Math.max(...lossHistory) : 0.035;
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

      {/* AI Learning & Health Summary Badges (Matching Left Radar Layout) */}
      <div className="border-t border-slate-800 pt-3">
        <div className="text-[11px] font-semibold text-slate-400 mb-2">
          オンデバイス学習・異常検知ステータス:
        </div>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs">
          {/* Phase */}
          <div className="p-2 rounded-lg border bg-slate-800/40 border-slate-700/40 text-slate-300 flex flex-col justify-between">
            <div className="text-[11px] text-slate-400">学習フェーズ</div>
            <div className="font-mono font-bold text-sm mt-1 text-emerald-400">
              PHASE {phase}
            </div>
            <div className="text-[10px] text-slate-500">
              {phase === 2 ? '自律監視 & ODL' : phase === 1 ? '日周適応中' : 'プロファイリング'}
            </div>
          </div>

          {/* Train Count */}
          <div className="p-2 rounded-lg border bg-slate-800/40 border-slate-700/40 text-slate-300 flex flex-col justify-between">
            <div className="text-[11px] text-slate-400">累積学習回数</div>
            <div className="font-mono font-bold text-sm mt-1 text-cyan-300">
              {trainCount} <span className="text-[10px] font-normal text-slate-400">回</span>
            </div>
            <div className="text-[10px] text-slate-500">オンデバイスRLS</div>
          </div>

          {/* Current Loss */}
          <div className={`p-2 rounded-lg border flex flex-col justify-between ${
            currentLoss > ANOMALY_THRESHOLD
              ? 'bg-rose-950/30 border-rose-500/40 text-rose-300'
              : 'bg-slate-800/40 border-slate-700/40 text-slate-300'
          }`}>
            <div className="text-[11px] text-slate-400">再構成損失</div>
            <div className="font-mono font-bold text-sm mt-1 text-emerald-400">
              {currentLoss.toFixed(4)}
            </div>
            <div className="text-[10px] text-slate-500">
              {currentLoss <= 0.05 ? '正常収束 (Optimal)' : '要観察'}
            </div>
          </div>

          {/* Anomaly Score */}
          <div className={`p-2 rounded-lg border flex flex-col justify-between ${
            anomalyScore > 30
              ? 'bg-amber-950/30 border-amber-500/40 text-amber-300'
              : 'bg-slate-800/40 border-slate-700/40 text-slate-300'
          }`}>
            <div className="text-[11px] text-slate-400">AI異常スコア</div>
            <div className="font-mono font-bold text-sm mt-1 text-indigo-300">
              {anomalyScore} <span className="text-[10px] font-normal text-slate-400">/ 100</span>
            </div>
            <div className="text-[10px] text-slate-500">
              {anomalyScore < 30 ? '健全維持' : '潜在的乖離あり'}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
