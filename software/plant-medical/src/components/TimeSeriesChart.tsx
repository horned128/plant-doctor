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

interface TimeSeriesChartProps {
  timeLabels: string[];
  airTemps: number[];
  leafTemps: number[];
  soilRaws: number[];
  onExportCsv: () => void;
  isHistory?: boolean;
}

export const TimeSeriesChart: React.FC<TimeSeriesChartProps> = ({
  timeLabels,
  airTemps,
  leafTemps,
  soilRaws,
  onExportCsv,
  isHistory = false,
}) => {
  const tempData = {
    labels: timeLabels,
    datasets: [
      {
        label: '葉温 (Leaf Temp)',
        data: leafTemps,
        borderColor: '#10b981',
        backgroundColor: 'rgba(16, 185, 129, 0.1)',
        tension: 0.3,
        fill: true,
      },
      {
        label: '気温 (Air Temp)',
        data: airTemps,
        borderColor: '#38bdf8',
        backgroundColor: 'transparent',
        borderDash: [5, 5],
        tension: 0.3,
      },
    ],
  };

  const soilData = {
    labels: timeLabels,
    datasets: [
      {
        label: '土壌水分生値 (Raw)',
        data: soilRaws,
        borderColor: '#0284c7',
        backgroundColor: 'rgba(2, 132, 199, 0.1)',
        tension: 0.3,
        fill: true,
      },
    ],
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      x: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#94a3b8', font: { size: 10 }, maxRotation: 0 },
      },
      y: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#94a3b8', font: { size: 10 } },
      },
    },
    plugins: {
      legend: {
        labels: { color: '#cbd5e1', font: { size: 11 } },
      },
    },
  };

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h2 className="text-sm font-bold text-slate-300 uppercase tracking-wider flex items-center gap-2">
          <span>
            {isHistory
              ? '📈 過去生体 & 環境推移グラフ (History Range)'
              : '📈 リアルタイム生体 & 環境推移グラフ (Live)'}
          </span>
        </h2>
        <button
          onClick={onExportCsv}
          className="text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 px-3 py-1.5 rounded-xl border border-slate-700 flex items-center gap-1.5 transition"
        >
          <span>📥 測定データ CSV エクスポート</span>
        </button>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="bg-slate-900/90 rounded-2xl p-6 border border-slate-800 shadow-xl">
          <h3 className="text-xs font-bold text-slate-400 mb-4 uppercase">
            気温 &bull; 葉温比較 (&deg;C)
          </h3>
          <div className="h-64">
            <Line data={tempData} options={chartOptions} />
          </div>
        </div>

        <div className="bg-slate-900/90 rounded-2xl p-6 border border-slate-800 shadow-xl">
          <h3 className="text-xs font-bold text-slate-400 mb-4 uppercase">
            土壌水分 Raw 値推移
          </h3>
          <div className="h-64">
            <Line data={soilData} options={chartOptions} />
          </div>
        </div>
      </div>
    </div>
  );
};
