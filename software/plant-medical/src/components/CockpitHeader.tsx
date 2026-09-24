import React, { useState } from 'react';
import { ConnectionState, PlantProfile } from '../types';
import {
  Leaf,
  Activity,
  LineChart,
  Cpu,
  Settings,
  Edit2,
  Check,
  Sparkles,
  WifiOff,
} from 'lucide-react';

export type CockpitTab = 'cockpit' | 'history' | 'solist' | 'system';

interface CockpitHeaderProps {
  activeTab: CockpitTab;
  onChangeTab: (tab: CockpitTab) => void;
  activeProfile: PlantProfile;
  profiles: PlantProfile[];
  onSelectProfile: (id: string) => void;
  onUpdateProfileName: (id: string, newName: string) => void;
  onTrainPersonalModel: (profileId: string) => void;
  isTrainingPersonal: boolean;
  connState: ConnectionState;
}

export const CockpitHeader: React.FC<CockpitHeaderProps> = ({
  activeTab,
  onChangeTab,
  activeProfile,
  profiles,
  onSelectProfile,
  onUpdateProfileName,
  onTrainPersonalModel,
  isTrainingPersonal,
  connState,
}) => {
  const [isEditingName, setIsEditingName] = useState(false);
  const [tempName, setTempName] = useState(activeProfile.name);

  const handleSaveName = () => {
    if (tempName.trim()) {
      onUpdateProfileName(activeProfile.id, tempName.trim());
    }
    setIsEditingName(false);
  };

  return (
    <header className="w-full bg-slate-950/70 border-b border-slate-800/80 backdrop-blur-md px-4 py-2.5 flex flex-col lg:flex-row items-center justify-between gap-3 select-none">
      {/* Brand & Plant Identity */}
      <div className="flex flex-wrap items-center justify-between w-full lg:w-auto gap-3">
        {/* Logo */}
        <div className="flex items-center space-x-2.5">
          <div className="w-8 h-8 rounded-xl bg-gradient-to-tr from-emerald-500 to-teal-400 text-slate-950 flex items-center justify-center shadow-lg shadow-emerald-500/20 font-black">
            <Leaf className="w-4 h-4 text-slate-950" />
          </div>
          <div>
            <div className="flex items-center gap-1.5">
              <h1 className="text-sm font-black tracking-tight text-white uppercase">
                Plant Medical
              </h1>
              <span className="text-[9px] font-mono font-bold px-1.5 py-0.2 rounded bg-emerald-950 text-emerald-400 border border-emerald-800">
                CLINIC
              </span>
            </div>
            <p className="text-[10px] text-slate-400 font-sans hidden sm:block">
              植物医療・生体診察ステーション
            </p>
          </div>
        </div>

        <div className="h-4 w-[1px] bg-slate-800 hidden sm:block" />

        {/* Plant Profile Capsule */}
        <div className="flex items-center space-x-2 bg-slate-900/80 px-2.5 py-1 rounded-xl border border-slate-800 text-xs">
          {/* Plant Name (Editable) */}
          {isEditingName ? (
            <div className="flex items-center space-x-1">
              <input
                type="text"
                value={tempName}
                onChange={(e) => setTempName(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && handleSaveName()}
                className="bg-slate-800 border border-emerald-500 text-white text-xs font-bold px-1.5 py-0.5 rounded focus:outline-none w-24"
                autoFocus
              />
              <button
                onClick={handleSaveName}
                className="p-1 bg-emerald-600 hover:bg-emerald-500 text-white rounded cursor-pointer"
              >
                <Check className="w-3 h-3" />
              </button>
            </div>
          ) : (
            <div className="flex items-center space-x-1 group">
              <span className="font-bold text-white text-xs tracking-tight">
                {activeProfile.name}
              </span>
              <button
                onClick={() => {
                  setTempName(activeProfile.name);
                  setIsEditingName(true);
                }}
                className="p-0.5 text-slate-500 hover:text-emerald-300 transition"
                title="植物の名前を編集"
              >
                <Edit2 className="w-3 h-3" />
              </button>
            </div>
          )}

          <span className="text-slate-700">&bull;</span>

          {/* Species Selector */}
          <select
            value={activeProfile.id}
            onChange={(e) => onSelectProfile(e.target.value)}
            className="bg-transparent border-none text-[11px] font-semibold text-slate-300 focus:outline-none cursor-pointer"
          >
            {profiles.map((p) => (
              <option key={p.id} value={p.id} className="bg-slate-900 text-slate-200">
                {p.species === 'pothos'
                  ? '🌿 ポトス'
                  : p.species === 'spathiphyllum'
                  ? '🌸 スパティフラム'
                  : '🪴 カスタム'}
              </option>
            ))}
          </select>

          {/* Personal Adaptive Learning Button */}
          <button
            onClick={() => onTrainPersonalModel(activeProfile.id)}
            disabled={isTrainingPersonal}
            className="flex items-center space-x-1 px-2 py-0.5 rounded-lg text-[10px] font-bold bg-emerald-600/20 hover:bg-emerald-600/40 text-emerald-300 border border-emerald-500/40 transition cursor-pointer disabled:opacity-50"
            title="現在の安定環境データをこの植物の個別生体モデルとして学習記憶"
          >
            <Sparkles className={`w-2.5 h-2.5 text-amber-300 ${isTrainingPersonal ? 'animate-spin' : ''}`} />
            <span>{isTrainingPersonal ? '学習中' : 'モデル適応'}</span>
          </button>
        </div>
      </div>

      {/* Navigation Buttons (Cockpit Mode Switches: 4 Unified Menus) */}
      <nav className="flex items-center gap-1 bg-slate-900/90 p-1 rounded-2xl border border-slate-800">
        {/* Menu 1: Medical Cockpit (NOW) */}
        <button
          onClick={() => onChangeTab('cockpit')}
          className={`flex items-center space-x-1.5 px-3.5 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
            activeTab === 'cockpit'
              ? 'bg-gradient-to-r from-emerald-600 to-teal-600 text-white shadow-md shadow-emerald-600/30'
              : 'text-slate-400 hover:text-slate-200'
          }`}
        >
          <Activity className="w-3.5 h-3.5" />
          <span>診察室 (Now)</span>
        </button>

        {/* Menu 2: History & Analytics (UNIFIED: 時系列推移 ＆ 履歴ログ) */}
        <button
          onClick={() => onChangeTab('history')}
          className={`flex items-center space-x-1.5 px-3.5 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
            activeTab === 'history'
              ? 'bg-gradient-to-r from-teal-600 to-cyan-600 text-white shadow-md shadow-teal-600/30'
              : 'text-slate-400 hover:text-slate-200'
          }`}
        >
          <LineChart className="w-3.5 h-3.5" />
          <span>履歴・推移ログ</span>
        </button>

        {/* Menu 3: Solist-AI™ Lab */}
        <button
          onClick={() => onChangeTab('solist')}
          className={`flex items-center space-x-1.5 px-3.5 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
            activeTab === 'solist'
              ? 'bg-cyan-600 text-white shadow-md shadow-cyan-600/30'
              : 'text-slate-400 hover:text-slate-200'
          }`}
        >
          <Cpu className="w-3.5 h-3.5" />
          <span>Solist-AI™</span>
        </button>

        {/* Menu 4: System & Settings */}
        <button
          onClick={() => onChangeTab('system')}
          className={`flex items-center space-x-1.5 px-3.5 py-1.5 rounded-xl text-xs font-bold transition cursor-pointer ${
            activeTab === 'system'
              ? 'bg-slate-700 text-white shadow-md'
              : 'text-slate-400 hover:text-slate-200'
          }`}
        >
          <Settings className="w-3.5 h-3.5" />
          <span>システム</span>
        </button>
      </nav>

      {/* Connectivity Status Capsule */}
      <div className="flex items-center space-x-2">
        <div className="flex items-center space-x-1.5 bg-slate-900/80 px-2.5 py-1 rounded-xl border border-slate-800 text-[11px] font-mono">
          {connState === 'connected' ? (
            <span className="flex items-center gap-1.5 text-emerald-400">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              <span>ONLINE</span>
            </span>
          ) : connState === 'connecting' ? (
            <span className="flex items-center gap-1.5 text-amber-400">
              <span className="w-2 h-2 rounded-full bg-amber-400 animate-ping" />
              <span>CONNECTING</span>
            </span>
          ) : (
            <span className="flex items-center gap-1.5 text-rose-400">
              <WifiOff className="w-3 h-3" />
              <span>STANDALONE</span>
            </span>
          )}
        </div>
      </div>
    </header>
  );
};
