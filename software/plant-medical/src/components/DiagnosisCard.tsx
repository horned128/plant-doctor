import React from 'react';

interface DiagnosisCardProps {
  status: string;
  soilTrend: string;
}

const STATUS_MAP: Record<string, { title: string; desc: string; color: string; border: string; bg: string }> = {
  HEALTHY: {
    title: '正常・健康 (Healthy)',
    desc: '平常状態を維持しています。葉温と気温の蒸散バランス、土壌水分ともに安定しています。',
    color: 'text-emerald-400',
    border: 'border-emerald-800/80',
    bg: 'bg-emerald-950/20'
  },
  HEAT_STRESS: {
    title: '熱ストレス (Heat Stress)',
    desc: '日照または気温上昇に対し、葉温の過度な上昇が検出されました。遮光や通風が必要です。',
    color: 'text-amber-400',
    border: 'border-amber-800/80',
    bg: 'bg-amber-950/20'
  },
  DRY_STRESS: {
    title: '乾燥ストレス (Dry Stress)',
    desc: '土壌水分の低下および蒸散抑制傾向が検出されました。水やりが推奨されます。',
    color: 'text-orange-400',
    border: 'border-orange-800/80',
    bg: 'bg-orange-950/20'
  },
  WATERING: {
    title: '水やり中 (Watering)',
    desc: 'ポンプが動作中です。給水後の土壌水分応答を監視しています。',
    color: 'text-sky-400',
    border: 'border-sky-800/80',
    bg: 'bg-sky-950/20'
  },
  WATERING_FAILED: {
    title: '給水失敗 (Watering Failed)',
    desc: 'ポンプ動作後に土壌水分の回復が確認できませんでした。タンク空やチューブ外れを確認してください。',
    color: 'text-rose-400',
    border: 'border-rose-800/80',
    bg: 'bg-rose-950/20'
  },
  SOIL_SENSOR_ERROR: {
    title: 'センサ異常 (Soil Sensor Error)',
    desc: '土壌水分センサの急激な離脱または接触不良が疑われます。設置を確認してください。',
    color: 'text-purple-400',
    border: 'border-purple-800/80',
    bg: 'bg-purple-950/20'
  },
};

export const DiagnosisCard: React.FC<DiagnosisCardProps> = ({ status, soilTrend }) => {
  const info = STATUS_MAP[status] || {
    title: status || 'UNKNOWN',
    desc: '状態を診断中またはデータ取得待機中です。',
    color: 'text-slate-300',
    border: 'border-slate-800',
    bg: 'bg-slate-900/50'
  };

  return (
    <div className={`rounded-2xl p-6 border shadow-xl flex flex-col justify-between ${info.bg} ${info.border}`}>
      <div>
        <div className="flex justify-between items-center mb-3">
          <span className="text-xs font-bold text-slate-400 uppercase tracking-wider">AI 総合診断</span>
          <span className="text-[10px] font-bold px-2 py-0.5 rounded bg-slate-800 text-slate-400 uppercase">
            R1 / R6
          </span>
        </div>

        <div className={`text-2xl font-black ${info.color} mb-2`}>
          {info.title}
        </div>
        <p className="text-xs text-slate-300 leading-relaxed">
          {info.desc}
        </p>
      </div>

      <div className="mt-6 pt-4 border-t border-slate-800/80 flex justify-between items-center text-xs">
        <span className="text-slate-400 font-medium">土壌水分傾向 (Soil Trend):</span>
        <span className="font-mono font-bold text-sky-400 bg-sky-950/80 px-2.5 py-1 rounded-lg border border-sky-800">
          {soilTrend || 'STABLE'}
        </span>
      </div>
    </div>
  );
};
