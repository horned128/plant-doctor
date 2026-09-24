import React from 'react';
import {
  Chart as ChartJS,
  RadialLinearScale,
  PointElement,
  LineElement,
  Filler,
  Tooltip,
  Legend,
} from 'chart.js';
import { Radar } from 'react-chartjs-2';
import { Compass, CheckCircle2, AlertTriangle } from 'lucide-react';

ChartJS.register(
  RadialLinearScale,
  PointElement,
  LineElement,
  Filler,
  Tooltip,
  Legend
);

import { PlantProfile } from '../types';

interface SolistAiFeatureRadarProps {
  soilRaw: number;
  leafTemp: number;
  airTemp: number;
  leafAirDiff: number;
  humidity: number;
  lux: number;
  leafTempRate?: number;
  soilRate?: number;
  activeProfile?: PlantProfile;
}

export const SolistAiFeatureRadar: React.FC<SolistAiFeatureRadarProps> = ({
  soilRaw,
  leafTemp,
  airTemp,
  leafAirDiff,
  humidity,
  lux,
  leafTempRate = 0.0,
  soilRate = 0.0,
  activeProfile,
}) => {
  // Normalize each feature to a 0-100 scale for intuitive radar comparison
  const clamp = (val: number, min: number, max: number) =>
    Math.min(Math.max(val, min), max);

  // 1. Soil Moisture (soilRaw: ~2400 dry, ~1200 wet; so 1800 is ~50%)
  const normSoil = clamp(
    Math.round(((2400 - soilRaw) / (2400 - 1200)) * 100),
    0,
    100
  );

  // 2. Leaf Temp (15C -> 0, 25C -> 50, 35C -> 100)
  const normLeafTemp = clamp(Math.round(((leafTemp - 15) / 20) * 100), 0, 100);

  // 3. Air Temp (15C -> 0, 25C -> 50, 35C -> 100)
  const normAirTemp = clamp(Math.round(((airTemp - 15) / 20) * 100), 0, 100);

  // 4. Leaf-Air Diff (-4C -> 0, -1C -> 50, +2C -> 100)
  const normDiff = clamp(
    Math.round(((leafAirDiff - (-4.0)) / 6.0) * 100),
    0,
    100
  );

  // 5. Humidity (20% -> 0, 60% -> 50, 100% -> 100)
  const normHum = clamp(Math.round(((humidity - 20) / 80) * 100), 0, 100);

  // 6. Lux: 植物好適光量ゾーン（不感帯）を考慮した落とし込み
  // 室内照明（200〜800 Lux）は植物にとって快適な光合成光量であるため、50（適正中央）付近に収斂。
  // 人間の生活照明の点灯/消灯ノイズで異常判定されないよう、好適帯は 45〜55% に落ち着かせ、
  // <50 Lux（日照不足）や >1500 Lux（直射日光・葉焼け）のみをストレスとして表現。
  let normLux = 50;
  if (lux < 200) {
    normLux = clamp(Math.round((lux / 200) * 45), 5, 45);
  } else if (lux <= 850) {
    normLux = clamp(Math.round(45 + ((lux - 200) / 650) * 10), 45, 55);
  } else {
    normLux = clamp(Math.round(55 + ((lux - 850) / 1150) * 45), 55, 100);
  }

  // 7. Leaf Temp Rate (-5C/h -> 0, 0C/h -> 50, +5C/h -> 100)
  const normLeafRate = clamp(
    Math.round(((leafTempRate - (-5.0)) / 10.0) * 100),
    0,
    100
  );

  // 8. Soil Rate (-200 /h -> 0, 0 /h -> 50, +200 /h -> 100)
  const normSoilRate = clamp(
    Math.round(((soilRate - (-200)) / 400) * 100),
    0,
    100
  );

  const featureLabels = [
    '土壌水分',
    '葉面温度',
    '環境気温',
    '葉気温差 ΔT',
    '湿度',
    '照度 (Lux)',
    '葉温変化率',
    '土壌水分変化率',
  ];

  const currentValues = [
    normSoil,
    normLeafTemp,
    normAirTemp,
    normDiff,
    normHum,
    normLux,
    normLeafRate,
    normSoilRate,
  ];

  // 植物プロファイル固有の学習基準モデル（未指定時はデフォルト）
  const baselineValues = activeProfile?.baselineFeatures ?? [55, 50, 50, 45, 55, 50, 50, 50];

  const data = {
    labels: featureLabels,
    datasets: [
      {
        label: '現在の生体特徴量 (Current State)',
        data: currentValues,
        backgroundColor: 'rgba(56, 189, 248, 0.25)',
        borderColor: '#38bdf8',
        borderWidth: 2,
        pointBackgroundColor: '#38bdf8',
        pointBorderColor: '#fff',
        pointHoverBackgroundColor: '#fff',
        pointHoverBorderColor: '#38bdf8',
      },
      {
        label: activeProfile ? `${activeProfile.name} 基準モデル` : '学習基準モデル (Learned Baseline)',
        data: baselineValues,
        backgroundColor: 'rgba(16, 185, 129, 0.15)',
        borderColor: '#10b981',
        borderWidth: 1.5,
        borderDash: [4, 4],
        pointBackgroundColor: '#10b981',
        pointBorderColor: '#fff',
      },
    ],
  };

  const options: any = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      r: {
        min: 0,
        max: 100,
        ticks: {
          display: false,
          stepSize: 25,
        },
        grid: {
          color: 'rgba(51, 65, 85, 0.5)',
        },
        angleLines: {
          color: 'rgba(51, 65, 85, 0.6)',
        },
        pointLabels: {
          color: '#cbd5e1',
          font: {
            size: 11,
            family: 'sans-serif',
          },
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
        },
      },
      tooltip: {
        backgroundColor: 'rgba(15, 23, 42, 0.95)',
        borderColor: '#334155',
        borderWidth: 1,
        titleColor: '#e2e8f0',
        bodyColor: '#cbd5e1',
        callbacks: {
          label: (context: any) =>
            ` ${context.dataset.label}: ${context.parsed.r}% (正規化値)`,
        },
      },
    },
  };

  // Attribution indicators
  const luxBaseline = baselineValues[5] ?? 50;
  const featureDetails = [
    { label: '土壌水分', raw: `${normSoil}%`, diff: Math.abs(normSoil - (baselineValues[0] ?? 55)) },
    { label: '葉気温差 ΔT', raw: `${leafAirDiff > 0 ? '+' : ''}${leafAirDiff.toFixed(1)}℃`, diff: Math.abs(normDiff - (baselineValues[3] ?? 45)) },
    { label: '照度環境', raw: `${lux} Lux`, diff: Math.abs(normLux - luxBaseline) },
    { label: '土壌変化率', raw: `${soilRate.toFixed(0)}/h`, diff: Math.abs(normSoilRate - (baselineValues[7] ?? 50)) },
  ];

  return (
    <div className="bg-slate-900/90 border border-slate-700/80 rounded-2xl p-6 shadow-xl backdrop-blur-sm flex flex-col justify-between">
      {/* Header */}
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center space-x-2">
          <div className="p-2 bg-cyan-500/10 border border-cyan-500/30 rounded-lg text-cyan-400">
            <Compass className="w-5 h-5" />
          </div>
          <div>
            <h3 className="text-base font-bold text-white tracking-wide">
              8次元 生体特徴空間レーダー
            </h3>
            <p className="text-xs text-slate-400">
              Solist-AI™ Autoencoder 8-Dimensional Input Space
            </p>
          </div>
        </div>
      </div>

      {/* Radar Canvas */}
      <div className="h-64 w-full relative mb-4">
        <Radar data={data} options={options} />
      </div>

      {/* Factor Attribution Badges */}
      <div className="border-t border-slate-800 pt-3">
        <div className="text-[11px] font-semibold text-slate-400 mb-2">
          主要要因の乖離度分析 (Factor Attribution):
        </div>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs">
          {featureDetails.map((f, i) => {
            const isWarning = f.diff > 25;
            return (
              <div
                key={i}
                className={`p-2 rounded-lg border flex flex-col justify-between ${
                  isWarning
                    ? 'bg-amber-950/30 border-amber-500/40 text-amber-300'
                    : 'bg-slate-800/40 border-slate-700/40 text-slate-300'
                }`}
              >
                <div className="flex items-center justify-between text-[11px] text-slate-400">
                  <span>{f.label}</span>
                  {isWarning ? (
                    <AlertTriangle className="w-3 h-3 text-amber-400" />
                  ) : (
                    <CheckCircle2 className="w-3 h-3 text-emerald-400" />
                  )}
                </div>
                <div className="font-mono font-bold text-sm mt-1">{f.raw}</div>
                <div className="text-[10px] text-slate-500">
                  {isWarning ? '基準値乖離大' : '基準値近傍'}
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
};
