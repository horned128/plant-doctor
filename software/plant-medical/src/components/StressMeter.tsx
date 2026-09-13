import React from 'react';

interface StressMeterProps {
  score: number;
}

export const StressMeter: React.FC<StressMeterProps> = ({ score }) => {
  const circumference = 251.2;
  const offset = circumference - (Math.min(Math.max(score, 0), 100) / 100) * circumference;

  let strokeColor = '#10b981'; // Green
  let label = '平常・安定 (Healthy)';
  let badgeClass = 'bg-emerald-950/80 text-emerald-400 border-emerald-800';

  if (score >= 66) {
    strokeColor = '#ef4444'; // Red
    label = '高ストレス警告 (Critical)';
    badgeClass = 'bg-rose-950/80 text-rose-400 border-rose-800';
  } else if (score >= 31) {
    strokeColor = '#f59e0b'; // Amber
    label = '軽度ストレス (Caution)';
    badgeClass = 'bg-amber-950/80 text-amber-400 border-amber-800';
  }

  return (
    <div className="bg-slate-900/90 rounded-2xl p-6 border border-slate-800 shadow-xl flex flex-col items-center justify-between relative overflow-hidden">
      <div className="w-full flex justify-between items-center mb-2">
        <span className="text-xs font-bold text-slate-400 uppercase tracking-wider">植物ストレス度</span>
        <span className="text-[11px] font-mono text-slate-500">Solist-AI™ R3</span>
      </div>

      <div className="relative w-44 h-44 flex items-center justify-center my-3">
        <svg className="w-full h-full transform -rotate-90" viewBox="0 0 100 100">
          <circle
            cx="50"
            cy="50"
            r="40"
            stroke="currentColor"
            strokeWidth="8"
            className="text-slate-800 fill-none"
          />
          <circle
            cx="50"
            cy="50"
            r="40"
            stroke={strokeColor}
            strokeWidth="8"
            className="gauge-transition fill-none"
            strokeLinecap="round"
            strokeDasharray="251.2"
            strokeDashoffset={offset}
          />
        </svg>

        <div className="absolute flex flex-col items-center">
          <span className="text-5xl font-black text-white tracking-tight">{score}</span>
          <span className="text-[11px] text-slate-400 font-semibold tracking-wider uppercase">/ 100</span>
        </div>
      </div>

      <div className={`text-xs font-bold px-4 py-1.5 rounded-full border ${badgeClass} tracking-wide`}>
        {label}
      </div>
    </div>
  );
};
