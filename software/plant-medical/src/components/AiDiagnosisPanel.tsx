import React from 'react';
import { Cpu, AlertCircle, CheckCircle2, ShieldCheck, ArrowRight, Flame, Droplets, Sparkles, SunMedium } from 'lucide-react';

interface AiDiagnosisPanelProps {
  status: string;
  stress: number;
  soilTrend: string;
  soilRaw: number;
  leafAirDiff: number;
  airTemp: number;
  lux: number;
  aiLoss?: number;
  aiPhase?: number;
  aiTrainCount?: number;
  onNavigateToAiLab?: () => void;
}

const STATUS_CONFIGS: Record<
  string,
  {
    title: string;
    sub: string;
    desc: string;
    primaryFactor: string;
    factorDesc: string;
    icon: any;
    color: string;
    badgeBg: string;
    border: string;
  }
> = {
  HEALTHY: {
    title: '正常・健康 (Healthy)',
    sub: '生体恒常性維持',
    desc: '蒸散冷却バランス・土壌水分ともに極めて安定。生体恒常性が保たれています。',
    primaryFactor: '生体恒常性・光合成調和',
    factorDesc: '葉温-気温差（ΔT < 0）による活発な蒸散と十分な根圏水分を維持',
    icon: CheckCircle2,
    color: 'text-emerald-400',
    badgeBg: 'bg-emerald-950/80 text-emerald-400 border-emerald-700/60',
    border: 'border-emerald-800/40',
  },
  HEAT_STRESS: {
    title: '熱ストレス (Heat Stress)',
    sub: '蒸散冷却の飽和限界',
    desc: '気温または光量過多に対し葉温が上昇。気孔開度による放熱が限界に達しています。',
    primaryFactor: '葉温上昇 & 放熱能力超過',
    factorDesc: '葉温-気温差ΔTが正転（気孔閉鎖）し、葉組織内に熱が蓄積中',
    icon: Flame,
    color: 'text-amber-400',
    badgeBg: 'bg-amber-950/80 text-amber-400 border-amber-700/60',
    border: 'border-amber-800/40',
  },
  DRY_STRESS: {
    title: '乾燥ストレス (Dry Stress)',
    sub: '土壌水分欠乏・水切れ予兆',
    desc: '根圏水分の低下を検知。植物が気孔を閉じて蒸散を抑え、葉温が上昇傾向です。',
    primaryFactor: '根圏土壌水分の枯渇',
    factorDesc: '土壌Raw値が上昇（乾燥域）し、給水が必要と判定',
    icon: Droplets,
    color: 'text-orange-400',
    badgeBg: 'bg-orange-950/80 text-orange-400 border-orange-700/60',
    border: 'border-orange-800/40',
  },
  WATERING: {
    title: '給水動作中 (Watering Active)',
    sub: '水分浸透 & 生体応答監視',
    desc: '自動または手動給水シーケンスが実行中。土壌への浸透と生体の回復を追跡しています。',
    primaryFactor: 'ポンプ駆動・根圏浸透',
    factorDesc: '土壌水分Rawの急激な低下（湿潤化）と回復応答を監視中',
    icon: Sparkles,
    color: 'text-sky-400',
    badgeBg: 'bg-sky-950/80 text-sky-400 border-sky-700/60',
    border: 'border-sky-800/40',
  },
  WATERING_FAILED: {
    title: '給水失敗警告 (Watering Failed)',
    sub: '土壌水分不応',
    desc: 'ポンプ駆動後に土壌水分の回復が検出されませんでした。タンク空やチューブ外れを確認してください。',
    primaryFactor: '水供給経路の不全',
    factorDesc: '給水シーケンス完了後も土壌水分Rawに改善が見られず',
    icon: AlertCircle,
    color: 'text-rose-400',
    badgeBg: 'bg-rose-950/80 text-rose-400 border-rose-700/60',
    border: 'border-rose-800/40',
  },
  SOIL_SENSOR_ERROR: {
    title: 'センサ異常 (Sensor Anomaly)',
    sub: '電極接触不良の疑い',
    desc: '土壌水分センサの測定値に非連続な急変が検出されました。プローブの接触を確認してください。',
    primaryFactor: 'センサ入力信号の急変',
    factorDesc: 'AD変換値が許容物理限界値を超脱',
    icon: AlertCircle,
    color: 'text-purple-400',
    badgeBg: 'bg-purple-950/80 text-purple-400 border-purple-700/60',
    border: 'border-purple-800/40',
  },
  LOW_LIGHT: {
    title: '日照不足 (Low Light)',
    sub: '受光量欠乏・暗黒継続',
    desc: '24時間の積算受光量が不足しています（100 Lux未満が継続）。育成ライトの照射や窓際への移動を推奨します。',
    primaryFactor: '積算受光量の不足',
    factorDesc: '1日を通じた積算照度が好適下限値を下回り、光合成活性が著しく低下',
    icon: SunMedium,
    color: 'text-yellow-400',
    badgeBg: 'bg-yellow-950/80 text-yellow-400 border-yellow-700/60',
    border: 'border-yellow-800/40',
  },
};

export const AiDiagnosisPanel: React.FC<AiDiagnosisPanelProps> = ({
  status,
  stress,
  soilTrend,
  soilRaw,
  leafAirDiff,
  airTemp,
  lux,
  aiLoss = 0.0215,
  aiPhase = 2,
  aiTrainCount = 52,
  onNavigateToAiLab,
}) => {
  const cfg = STATUS_CONFIGS[status] || {
    title: status || 'UNKNOWN',
    sub: '状態診断待機中',
    desc: '生体テレメトリを受信・解析しています。',
    primaryFactor: 'データ蓄積中',
    factorDesc: '十分なサンプリングデータを収束中',
    icon: AlertCircle,
    color: 'text-slate-300',
    badgeBg: 'bg-slate-800 text-slate-400 border-slate-700',
    border: 'border-slate-800',
  };

  // Dial arc math for 0 to 100
  const circumference = 251.2;
  const clampedStress = Math.min(Math.max(stress, 0), 100);
  const offset = circumference - (clampedStress / 100) * circumference;

  let stressStroke = '#10b981'; // green
  if (clampedStress >= 60) {
    stressStroke = '#ef4444'; // red
  } else if (clampedStress >= 30) {
    stressStroke = '#f59e0b'; // amber
  }

  // Solist-AI phase name
  const phaseNames = ['プロファイリング', '適応・安定化', '自律監視 & 逐次学習'];
  const phaseName = phaseNames[Math.min(Math.max(aiPhase, 0), 2)];

  // Loss health (threshold is 0.080)
  const lossPercent = Math.min(100, Math.round((aiLoss / 0.08) * 100));

  const StatusIcon = cfg.icon;

  return (
    <div className="h-full flex flex-col justify-between bg-slate-950/40 border border-slate-800/60 rounded-3xl p-5 backdrop-blur-md relative overflow-hidden shadow-2xl">
      {/* Background Accent Subtle Glow */}
      <div className="absolute -top-16 -right-16 w-44 h-44 bg-emerald-500/10 rounded-full blur-3xl pointer-events-none" />

      {/* Header */}
      <div className="flex items-center justify-between border-b border-slate-800/80 pb-3">
        <div className="flex items-center space-x-2">
          <div className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
          <h3 className="text-xs font-bold uppercase tracking-widest text-slate-300">
            植物生体 診断レポート
          </h3>
        </div>
        <span className="text-[10px] text-emerald-400 bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-800/50 flex items-center gap-1">
          <ShieldCheck className="w-3 h-3" />
          AI 生体状態推論
        </span>
      </div>

      {/* Stress Gauge & Primary Verdict */}
      <div className="my-3 flex flex-col sm:flex-row items-center gap-4 p-4 rounded-2xl bg-gradient-to-br from-slate-900/90 via-slate-900/60 to-slate-950/80 border border-slate-800/90 shadow-inner">
        {/* Stress Dial */}
        <div className="relative w-28 h-28 flex-shrink-0 flex items-center justify-center">
          <svg className="w-full h-full transform -rotate-90" viewBox="0 0 100 100">
            <circle
              cx="50"
              cy="50"
              r="40"
              stroke="currentColor"
              strokeWidth="7"
              className="text-slate-800/80 fill-none"
            />
            <circle
              cx="50"
              cy="50"
              r="40"
              stroke={stressStroke}
              strokeWidth="7"
              className="gauge-transition fill-none"
              strokeLinecap="round"
              strokeDasharray="251.2"
              strokeDashoffset={offset}
            />
          </svg>
          <div className="absolute flex flex-col items-center">
            <span className="text-3xl font-black text-white tracking-tight font-mono">
              {clampedStress}
            </span>
            <span className="text-[9px] text-slate-400 font-bold uppercase tracking-wider">
              STRESS
            </span>
          </div>
        </div>

        {/* Verdict Details */}
        <div className="flex-1 space-y-1 text-center sm:text-left">
          <div className="flex items-center justify-center sm:justify-start gap-1.5">
            <StatusIcon className={`w-4 h-4 ${cfg.color}`} />
            <span className={`text-base font-black tracking-tight ${cfg.color}`}>
              {cfg.title}
            </span>
          </div>
          <p className="text-[11px] text-slate-300 leading-relaxed font-sans">
            {cfg.desc}
          </p>
        </div>
      </div>

      {/* Why? - 主要要因分析 (Factor Attribution & Causality) */}
      <div className="p-3.5 rounded-2xl bg-slate-900/60 border border-slate-800/80 my-1 space-y-2">
        <div className="flex items-center justify-between text-[11px]">
          <span className="font-bold text-slate-300 flex items-center gap-1">
            <span className="text-teal-400 font-mono">Why?</span> AI診断の主要根拠
          </span>
          <span className="text-[10px] text-slate-500 font-mono">8D ATTRIBUTION</span>
        </div>

        <div className="p-2.5 rounded-xl bg-slate-950/70 border border-slate-800/70 space-y-1.5">
          <div className="flex items-center justify-between text-xs font-semibold">
            <span className={cfg.color}>{cfg.primaryFactor}</span>
            <span className="text-[10px] text-slate-400 font-mono">
              浸透: {soilTrend || 'STABLE'}
            </span>
          </div>
          <div className="text-[10px] text-slate-400 leading-normal">
            {cfg.factorDesc}
          </div>
          <div className="flex items-center justify-between text-[9px] font-mono text-slate-500 pt-1 border-t border-slate-800/60">
            <span>土壌Raw: {soilRaw}</span>
            <span>気温: {airTemp.toFixed(1)}℃ / 照度: {lux}lx</span>
            <span>ΔT: {leafAirDiff > 0 ? '+' : ''}{leafAirDiff.toFixed(2)}℃</span>
          </div>
        </div>
      </div>

      {/* Solist-AI™ On-Device Learning Mini Monitor */}
      <div className="mt-2 pt-2.5 border-t border-slate-800/80 space-y-2">
        <div className="flex items-center justify-between text-[10px] font-mono text-slate-400">
          <span className="flex items-center gap-1 text-slate-300">
            <Cpu className="w-3.5 h-3.5 text-teal-400" />
            <span>Solist-AI™ リアルタイム推論</span>
          </span>
          <span>学習 {aiTrainCount}回 &bull; {phaseName}</span>
        </div>

        {/* Loss Bar & Navigation trigger */}
        <div className="flex items-center justify-between gap-3 text-[10px] font-mono">
          <span className="text-slate-400">再構成損失 (MSE):</span>
          <div className="flex-1 bg-slate-900 h-1.5 rounded-full overflow-hidden">
            <div
              className={`h-full transition-all duration-500 rounded-full ${
                aiLoss <= 0.05 ? 'bg-emerald-400' : aiLoss <= 0.08 ? 'bg-amber-400' : 'bg-rose-500'
              }`}
              style={{ width: `${Math.min(100, Math.max(5, lossPercent))}%` }}
            />
          </div>
          <span className="text-emerald-400 font-bold">{aiLoss.toFixed(4)}</span>
        </div>

        {onNavigateToAiLab && (
          <button
            onClick={onNavigateToAiLab}
            className="w-full flex items-center justify-center gap-1 text-[11px] font-semibold text-teal-400 hover:text-teal-300 pt-1 transition group cursor-pointer"
          >
            <span>8次元生体特徴空間 ＆ ODL詳細分析</span>
            <ArrowRight className="w-3.5 h-3.5 group-hover:translate-x-1 transition-transform" />
          </button>
        )}
      </div>
    </div>
  );
};
