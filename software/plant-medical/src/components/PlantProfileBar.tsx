import React, { useState } from 'react';
import { PlantProfile } from '../types';
import { Sprout, Edit2, Check, Sparkles, Sliders, ShieldCheck } from 'lucide-react';

interface PlantProfileBarProps {
  activeProfile: PlantProfile;
  profiles: PlantProfile[];
  onSelectProfile: (id: string) => void;
  onUpdateProfileName: (id: string, newName: string) => void;
  onTrainPersonalModel: (profileId: string) => void;
  isTraining?: boolean;
}

export const PlantProfileBar: React.FC<PlantProfileBarProps> = ({
  activeProfile,
  profiles,
  onSelectProfile,
  onUpdateProfileName,
  onTrainPersonalModel,
  isTraining = false,
}) => {
  const [isEditingName, setIsEditingName] = useState(false);
  const [tempName, setTempName] = useState(activeProfile.name);
  const [showDetails, setShowDetails] = useState(false);

  const handleSaveName = () => {
    if (tempName.trim()) {
      onUpdateProfileName(activeProfile.id, tempName.trim());
    }
    setIsEditingName(false);
  };

  return (
    <div className="bg-slate-900/90 border border-slate-800 rounded-xl px-4 py-2.5 shadow-md flex flex-col gap-2">
      <div className="flex flex-wrap items-center justify-between gap-3">
        {/* 左側: 植物アイコン & 名前 & 品種セレクター */}
        <div className="flex items-center space-x-3">
          <div className="p-1.5 bg-emerald-500/10 border border-emerald-500/30 rounded-lg text-emerald-400">
            <Sprout className="w-4 h-4" />
          </div>

          {/* 植物名（インライン編集） */}
          <div className="flex items-center space-x-2">
            <span className="text-[11px] text-slate-400 font-medium">植物名:</span>
            {isEditingName ? (
              <div className="flex items-center space-x-1">
                <input
                  type="text"
                  value={tempName}
                  onChange={(e) => setTempName(e.target.value)}
                  onKeyDown={(e) => e.key === 'Enter' && handleSaveName()}
                  className="bg-slate-800 border border-emerald-500 text-white text-sm font-bold px-2 py-0.5 rounded focus:outline-none"
                  autoFocus
                />
                <button
                  onClick={handleSaveName}
                  className="p-1 bg-emerald-600 hover:bg-emerald-500 text-white rounded transition cursor-pointer"
                >
                  <Check className="w-3 h-3" />
                </button>
              </div>
            ) : (
              <div className="flex items-center space-x-1.5 group">
                <span className="text-sm font-bold text-white tracking-tight">
                  {activeProfile.name}
                </span>
                <button
                  onClick={() => {
                    setTempName(activeProfile.name);
                    setIsEditingName(true);
                  }}
                  className="p-1 text-slate-500 hover:text-emerald-300 rounded transition"
                  title="植物の名前を編集"
                >
                  <Edit2 className="w-3 h-3" />
                </button>
              </div>
            )}
          </div>

          <span className="text-slate-700">|</span>

          {/* 品種セレクター */}
          <div className="flex items-center space-x-1.5">
            <span className="text-[11px] text-slate-400 font-medium">登録品種:</span>
            <select
              value={activeProfile.id}
              onChange={(e) => onSelectProfile(e.target.value)}
              className="bg-slate-800/90 border border-slate-700 text-xs font-bold text-slate-200 rounded-lg px-2 py-1 focus:outline-none focus:ring-1 focus:ring-emerald-500 cursor-pointer"
            >
              {profiles.map((p) => (
                <option key={p.id} value={p.id}>
                  {p.species === 'pothos'
                    ? '🌿 ポトス'
                    : p.species === 'spathiphyllum'
                    ? '🌸 スパティフラム'
                    : '🪴 その他の植物 (カスタム)'}
                </option>
              ))}
            </select>
          </div>
        </div>

        {/* 右側: 学習状態 & パーソナル学習ボタン */}
        <div className="flex items-center space-x-2.5 text-xs">
          <div className="hidden sm:flex items-center space-x-2 text-slate-400 font-mono text-[11px]">
            <span>学習蓄積: <strong className="text-emerald-400">{activeProfile.learnedSamples}回</strong></span>
            <span>基準MSE: <strong className="text-cyan-400">{activeProfile.baselineLoss.toFixed(4)}</strong></span>
          </div>

          {/* パーソナル学習ボタン */}
          <button
            onClick={() => onTrainPersonalModel(activeProfile.id)}
            disabled={isTraining}
            className="flex items-center space-x-1 px-3 py-1 rounded-lg text-xs font-bold bg-emerald-600 hover:bg-emerald-500 text-white shadow transition cursor-pointer disabled:opacity-50"
            title="現在の安定環境データをこの植物の個別パーソナル生体モデルとして記憶・更新"
          >
            <Sparkles className={`w-3 h-3 text-amber-300 ${isTraining ? 'animate-spin' : ''}`} />
            <span>{isTraining ? '学習中...' : 'パーソナル学習・適応'}</span>
          </button>

          {/* 詳細トグル */}
          <button
            onClick={() => setShowDetails(!showDetails)}
            className="p-1 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-slate-200 border border-slate-700 transition cursor-pointer"
            title="診断しきい値を表示"
          >
            <Sliders className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* 詳細アコーディオン (スリム表示) */}
      {showDetails && (
        <div className="pt-2 border-t border-slate-800 grid grid-cols-1 sm:grid-cols-3 gap-2 text-[11px]">
          <div className="bg-slate-950/60 p-2 rounded-lg border border-slate-800 flex items-center justify-between">
            <span className="text-slate-400 flex items-center gap-1">
              <ShieldCheck className="w-3 h-3 text-emerald-400" />
              土壌適正:
            </span>
            <span className="font-mono text-white font-bold">{activeProfile.thresholds.soilMaxRaw}〜{activeProfile.thresholds.soilMinRaw} Raw</span>
          </div>
          <div className="bg-slate-950/60 p-2 rounded-lg border border-slate-800 flex items-center justify-between">
            <span className="text-slate-400">適正温度 / 照度:</span>
            <span className="font-mono text-white font-bold">{activeProfile.thresholds.tempMin}〜{activeProfile.thresholds.tempMax}℃ / {activeProfile.thresholds.luxMin}〜{activeProfile.thresholds.luxMax} lx</span>
          </div>
          <div className="bg-slate-950/60 p-2 rounded-lg border border-slate-800 text-slate-400 truncate">
            {activeProfile.description}
          </div>
        </div>
      )}
    </div>
  );
};
