import React from 'react';
import { ConnectionState } from '../types';

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
    <header className="flex flex-col md:flex-row justify-between items-center pb-6 border-b border-slate-800 mb-6 gap-4">
      <div className="flex items-center space-x-3">
        <div className="w-12 h-12 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 flex items-center justify-center text-3xl shadow-inner shadow-emerald-500/10">
          🌿
        </div>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="text-2xl font-black tracking-tight text-white">Plant Medical</h1>
            <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-emerald-950 text-emerald-400 border border-emerald-800 uppercase tracking-wide">
              Edge AI Telemetry
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">ROHM DT-EBML63Q2557 &times; ATOMS3 Lite Gateway</p>
        </div>
      </div>

      <div className="flex flex-wrap items-center gap-3">
        <div className="flex items-center space-x-2 bg-slate-900 px-3 py-1.5 rounded-xl border border-slate-800 text-xs">
          <span className="text-slate-400">Host:</span>
          <input
            type="text"
            value={gatewayHost}
            onChange={(e) => setGatewayHost(e.target.value)}
            placeholder="plant-doctor.local"
            className="bg-slate-800 text-slate-200 px-2 py-1 rounded border border-slate-700 w-36 outline-none focus:border-emerald-500 font-mono text-xs"
          />
        </div>

        <div className="flex items-center space-x-2 bg-slate-900 px-3 py-2 rounded-xl border border-slate-800 text-xs">
          <span
            className={`w-2.5 h-2.5 rounded-full ${
              connState === 'connected'
                ? 'bg-emerald-400 ring-4 ring-emerald-500/20'
                : connState === 'connecting'
                ? 'bg-amber-400 animate-pulse'
                : 'bg-rose-500 ring-4 ring-rose-500/20'
            }`}
          />
          <span className="text-slate-300 font-medium">
            {connState === 'connected' ? 'Live Connected' : connState === 'connecting' ? 'Connecting...' : 'Disconnected'}
          </span>
        </div>

        <button
          onClick={onSyncTime}
          className="text-xs bg-slate-800 hover:bg-slate-700 text-slate-200 px-3 py-2 rounded-xl border border-slate-700 font-medium transition active:scale-95"
        >
          🕒 RTC時刻同期
        </button>
      </div>
    </header>
  );
};
