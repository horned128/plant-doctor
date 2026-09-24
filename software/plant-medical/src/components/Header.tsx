import React from 'react';
import { ConnectionState } from '../types';
import { Leaf, Clock } from 'lucide-react';

interface HeaderProps {
  gatewayHost: string;
  setGatewayHost: (host: string) => void;
  connState: ConnectionState;
  onSyncTime: () => void;
}

export const Header: React.FC<HeaderProps> = ({
  gatewayHost,
  setGatewayHost,
  connState,
  onSyncTime,
}) => {
  return (
    <header className="flex flex-col sm:flex-row justify-between items-center pb-4 border-b border-slate-800/80 gap-3">
      {/* Logo & Brand */}
      <div className="flex items-center space-x-3 w-full sm:w-auto">
        <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-emerald-600 to-teal-500 text-white flex items-center justify-center shadow-lg shadow-emerald-600/20">
          <Leaf className="w-5 h-5" />
        </div>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="text-xl font-black tracking-tight text-white">Plant Medical</h1>
            <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-emerald-950/80 text-emerald-400 border border-emerald-800/60 uppercase tracking-wide">
              Station
            </span>
          </div>
          <p className="text-[11px] text-slate-400">ROHM DT-EBML63Q2557 &times; ATOMS3 Lite</p>
          <p className="text-[11px] text-slate-400">植物生体モニタリング ＆ 診察ステーション</p>
        </div>
      </div>

      {/* Connectivity & Actions */}
      <div className="flex flex-wrap items-center justify-end gap-2.5 w-full sm:w-auto">
        {/* Host config */}
        <div className="flex items-center space-x-1.5 bg-slate-900/90 px-2.5 py-1.5 rounded-xl border border-slate-800 text-xs">
          <span className="text-slate-500 text-[11px]">Host:</span>
          <input
            type="text"
            value={gatewayHost}
            onChange={(e) => setGatewayHost(e.target.value)}
            placeholder="plant-doctor.local"
            className="bg-slate-950 text-slate-200 px-2 py-0.5 rounded border border-slate-700/70 w-32 outline-none focus:border-emerald-500 font-mono text-[11px]"
          />
        </div>

        {/* Live Status Badge */}
        <div className="flex items-center space-x-2 bg-slate-900/90 px-3 py-1.5 rounded-xl border border-slate-800 text-xs">
          {connState === 'connected' ? (
            <span className="flex items-center gap-1.5 text-emerald-400 font-medium">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              <span>接続中</span>
            </span>
          ) : connState === 'connecting' ? (
            <span className="flex items-center gap-1.5 text-amber-400 font-medium">
              <span className="w-2 h-2 rounded-full bg-amber-400 animate-ping" />
              <span>接続試行...</span>
            </span>
          ) : (
            <span className="flex items-center gap-1.5 text-rose-400 font-medium">
              <span className="w-2 h-2 rounded-full bg-rose-500" />
              <span>未接続 (Demo)</span>
              <span>未接続 (オフライン)</span>
            </span>
          )}
        </div>

        {/* Sync RTC Button */}
        <button
          onClick={onSyncTime}
          className="flex items-center space-x-1 text-xs bg-slate-900 hover:bg-slate-800 text-slate-300 px-3 py-1.5 rounded-xl border border-slate-800 transition active:scale-95 cursor-pointer"
          title="PCの現在時刻をDT-EBMLのRTCへ同期"
        >
          <Clock className="w-3.5 h-3.5 text-slate-400" />
          <span>RTC同期</span>
        </button>
      </div>
    </header>
  );
};

