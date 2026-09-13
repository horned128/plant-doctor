import React from 'react';
import { Cpu, Activity, Award, ShieldCheck, Zap, Database } from 'lucide-react';

interface SolistAiStatusCardProps {
  phase?: number;
  trainCount?: number;
  loss?: number;
  anomalyScore?: number;
  stress: number;
}

export const SolistAiStatusCard: React.FC<SolistAiStatusCardProps> = ({
  phase = 0,
  trainCount = 0,
  loss = 0,
  anomalyScore = 0,
  stress,
}) => {
  // Phase definitions
  const phases = [
    {
      id: 0,
      name: 'プロファイリング',
      english: 'Profiling',
      desc: '初期環境・生体パターンの学習中',
      range: '0〜10回',
      color: 'text-amber-400 border-amber-400/60 bg-amber-950/40',
    },
    {
      id: 1,
      name: '適応・安定化',
      english: 'Stabilizing',
      desc: '日周リズムへの適応とベースライン収束',
      range: '11〜50回',
      color: 'text-cyan-400 border-cyan-400/60 bg-cyan-950/40',
    },
    {
      id: 2,
      name: '自律監視 & 逐次学習',
      english: 'Monitoring & ODL',
      desc: 'リアルタイム異常検知 & 健全時自己更新',
      range: '51回〜',
      color: 'text-emerald-400 border-emerald-400/60 bg-emerald-950/40',
    },
  ];

  const currentPhaseIndex = Math.min(Math.max(phase, 0), 2);
  const currentPhase = phases[currentPhaseIndex];
  const progressPercent = Math.min(100, Math.round((trainCount / 50) * 100));

  // Loss state classification
  const isHealthyTraining = stress < 30;
  const getLossBadge = (val: number) => {
    if (val <= 0.05) {
      return { text: '正常収束 (Optimal)', bg: 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40' };
    } else if (val <= 0.08) {
      return { text: '学習追従中 (Adapting)', bg: 'bg-amber-500/20 text-amber-400 border-amber-500/40' };
    } else {
      return { text: '再構成誤差増大 (Anomaly Alert)', bg: 'bg-rose-500/20 text-rose-400 border-rose-500/40' };
    }
  };

  const lossBadge = getLossBadge(loss);

  return (
    <div className="bg-slate-900/90 border border-slate-700/80 rounded-2xl p-6 shadow-xl backdrop-blur-sm relative overflow-hidden">
      {/* Subtle Background Glow */}
      <div className="absolute -top-12 -right-12 w-48 h-48 bg-emerald-500/10 rounded-full blur-3xl pointer-events-none" />

      {/* Header */}
      <div className="flex flex-wrap items-center justify-between gap-2 border-b border-slate-800 pb-4 mb-5">
        <div className="flex items-center space-x-3">
          <div className="p-2.5 bg-gradient-to-br from-emerald-500/20 to-cyan-500/20 border border-emerald-500/30 rounded-xl text-emerald-400 shadow-inner">
            <Cpu className="w-6 h-6 animate-pulse" />
          </div>
          <div>
            <div className="flex items-center space-x-2">
              <h3 className="text-lg font-bold text-white tracking-wide">
                Solist-AI™ オンデバイス学習ステーション
              </h3>
              <span className="px-2 py-0.5 text-xs font-semibold uppercase tracking-wider rounded-full bg-emerald-950 text-emerald-300 border border-emerald-600/40">
                ROHM ML63Q2557
              </span>
            </div>
            <p className="text-xs text-slate-400 mt-0.5">
              3層オートエンコーダ (8-64-8) によるリアルタイムオンデバイス学習・推論
            </p>
          </div>
        </div>

        {/* Learning Gate Badge */}
        <div className="flex items-center space-x-2">
          {isHealthyTraining ? (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-lg bg-emerald-950/60 border border-emerald-500/50 text-emerald-300 text-xs font-medium">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-ping" />
              <span>逐次学習稼働中 (Stress &lt; 30)</span>
            </div>
          ) : (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-lg bg-amber-950/60 border border-amber-500/50 text-amber-300 text-xs font-medium">
              <span className="w-2 h-2 rounded-full bg-amber-400" />
              <span>学習保留中 (ストレス検知中)</span>
            </div>
          )}
        </div>
      </div>

      {/* Phase Progression Stepper */}
      <div className="mb-6">
        <div className="flex items-center justify-between text-xs text-slate-400 mb-2 font-mono">
          <span className="text-slate-300 font-semibold">
            学習フェーズ進行度: <span className="text-emerald-400 font-bold">{currentPhase.name}</span> ({currentPhase.english})
          </span>
          <span>{trainCount} 回学習済 / 安定基準 50回 ({progressPercent}%)</span>
        </div>

        {/* Progress Bar */}
        <div className="w-full bg-slate-800 rounded-full h-2.5 overflow-hidden mb-4 p-0.5 border border-slate-700">
          <div
            className="bg-gradient-to-r from-cyan-500 to-emerald-400 h-full rounded-full transition-all duration-700 ease-out shadow-[0_0_12px_rgba(16,185,129,0.5)]"
            style={{ width: `${progressPercent}%` }}
          />
        </div>

        {/* Phase Badges */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
          {phases.map((p) => {
            const isActive = p.id === currentPhaseIndex;
            const isDone = trainCount >= (p.id === 0 ? 10 : 50);

            return (
              <div
                key={p.id}
                className={`p-3 rounded-xl border transition-all ${
                  isActive
                    ? `${p.color} ring-1 ring-emerald-400/50 shadow-lg`
                    : isDone
                    ? 'bg-slate-800/60 border-slate-700/60 text-slate-300'
                    : 'bg-slate-900/40 border-slate-800/40 text-slate-500'
                }`}
              >
                <div className="flex items-center justify-between mb-1">
                  <span className="text-xs font-bold font-mono tracking-wider">
                    PHASE {p.id}
                  </span>
                  <span className="text-[10px] font-mono opacity-80">{p.range}</span>
                </div>
                <div className="text-sm font-bold text-slate-100 mb-0.5">
                  {p.name}
                </div>
                <div className="text-[11px] leading-tight text-slate-400">{p.desc}</div>
              </div>
            );
          })}
        </div>
      </div>

      {/* Metrics Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 mb-5">
        {/* Metric 1: Train Count */}
        <div className="bg-slate-800/60 border border-slate-700/50 rounded-xl p-3.5 flex flex-col justify-between">
          <div className="flex items-center justify-between text-slate-400 text-xs mb-1">
            <span className="flex items-center gap-1.5">
              <Database className="w-3.5 h-3.5 text-cyan-400" />
              累積学習回数
            </span>
          </div>
          <div className="text-2xl font-black text-white font-mono tracking-tight">
            {trainCount}
            <span className="text-xs font-normal text-slate-400 ml-1">回 (steps)</span>
          </div>
          <div className="text-[11px] text-slate-400 mt-1">オンデバイス逐次更新</div>
        </div>

        {/* Metric 2: Reconstruction Loss */}
        <div className="bg-slate-800/60 border border-slate-700/50 rounded-xl p-3.5 flex flex-col justify-between">
          <div className="flex items-center justify-between text-slate-400 text-xs mb-1">
            <span className="flex items-center gap-1.5">
              <Activity className="w-3.5 h-3.5 text-emerald-400" />
              再構成損失 (MSE)
            </span>
          </div>
          <div className="text-2xl font-black text-emerald-400 font-mono tracking-tight">
            {loss.toFixed(4)}
          </div>
          <div className={`text-[10px] mt-1 px-1.5 py-0.5 rounded border inline-block ${lossBadge.bg}`}>
            {lossBadge.text}
          </div>
        </div>

        {/* Metric 3: Anomaly Score */}
        <div className="bg-slate-800/60 border border-slate-700/50 rounded-xl p-3.5 flex flex-col justify-between">
          <div className="flex items-center justify-between text-slate-400 text-xs mb-1">
            <span className="flex items-center gap-1.5">
              <ShieldCheck className="w-3.5 h-3.5 text-indigo-400" />
              AI異常スコア
            </span>
          </div>
          <div className="text-2xl font-black text-indigo-300 font-mono tracking-tight">
            {anomalyScore}
            <span className="text-xs font-normal text-slate-400 ml-1">/ 100</span>
          </div>
          <div className="text-[11px] text-slate-400 mt-1">
            {anomalyScore < 30 ? '健全状態を維持' : anomalyScore < 60 ? '軽度注意' : '生体異常検知'}
          </div>
        </div>

        {/* Metric 4: Forgetting Factor */}
        <div className="bg-slate-800/60 border border-slate-700/50 rounded-xl p-3.5 flex flex-col justify-between">
          <div className="flex items-center justify-between text-slate-400 text-xs mb-1">
            <span className="flex items-center gap-1.5">
              <Zap className="w-3.5 h-3.5 text-amber-400" />
              忘却係数 λ
            </span>
          </div>
          <div className="text-2xl font-black text-amber-300 font-mono tracking-tight">
            0.95
          </div>
          <div className="text-[11px] text-slate-400 mt-1">過去学習の減衰追従</div>
        </div>
      </div>

      {/* Tech Specifications Footer */}
      <div className="flex flex-wrap items-center justify-between text-[11px] text-slate-400 bg-slate-950/60 border border-slate-800/80 rounded-xl px-4 py-2.5 gap-2">
        <div className="flex items-center space-x-2">
          <Award className="w-3.5 h-3.5 text-cyan-400" />
          <span className="font-semibold text-slate-300">ハードウェアアクセラレータ:</span>
          <span className="font-mono text-cyan-300">ROHM ML63Q2557 AI_PERI (BFloat16 / Fixpoint)</span>
        </div>
        <div className="flex items-center space-x-4">
          <span>
            活性化関数: <strong className="text-slate-300">Hard Sigmoid</strong>
          </span>
          <span>
            損失基準: <strong className="text-slate-300">MSE Loss (PPM)</strong>
          </span>
          <span>
            データ同期: <strong className="text-emerald-400">I2C v2 (0x42)</strong>
          </span>
        </div>
      </div>
    </div>
  );
};
