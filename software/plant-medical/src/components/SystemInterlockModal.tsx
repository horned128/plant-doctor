import React from 'react';
import { Clock, ShieldCheck, Cpu, Network, Zap, CheckCircle2, AlertTriangle } from 'lucide-react';
import { ConnectionState } from '../types';

interface SystemInterlockModalProps {
  gatewayHost: string;
  setGatewayHost: (host: string) => void;
  connState: ConnectionState;
  onSyncTime: () => void;
  tankLiquid: boolean;
  pumpOn: boolean;
  seq?: number;
  timestamp?: number;
}

export const SystemInterlockModal: React.FC<SystemInterlockModalProps> = ({
  gatewayHost,
  setGatewayHost,
  connState,
  onSyncTime,
  tankLiquid,
  pumpOn,
  seq = 0,
  timestamp = 0,
}) => {
  const timeFormatted = timestamp ? new Date(timestamp * 1000).toLocaleString('ja-JP') : '--';

  return (
    <div className="max-w-5xl mx-auto space-y-6 py-2">
      {/* Upper Status Banner */}
      <div className="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-xl flex flex-col md:flex-row items-center justify-between gap-6">
        <div className="flex items-center space-x-4">
          <div className="p-3.5 bg-gradient-to-tr from-emerald-600 to-teal-500 rounded-2xl text-white shadow-lg shadow-emerald-500/20">
            <Cpu className="w-8 h-8" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h2 className="text-lg font-black text-white tracking-tight">
                ROHM DT-EBML63Q2557 生体計測システム
              </h2>
              <span className="px-2 py-0.5 rounded-full text-[10px] font-bold bg-emerald-950 text-emerald-400 border border-emerald-800 font-mono">
                ARM Cortex-M0+ &bull; Solist-AI™
              </span>
            </div>
            <p className="text-xs text-slate-400 mt-1">
              超低消費電力エッジAIマイコンによる植物生体常時モニタリング＆自律給水ステーション
            </p>
          </div>
        </div>

        {/* Live Connectivity Tag */}
        <div className="flex items-center gap-3 bg-slate-950/80 px-4 py-2.5 rounded-2xl border border-slate-800 text-xs font-mono">
          <span className="text-slate-400">通信状態:</span>
          {connState === 'connected' ? (
            <span className="text-emerald-400 font-bold flex items-center gap-1.5">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              ONLINE (WebSocket)
            </span>
          ) : (
            <span className="text-rose-400 font-bold flex items-center gap-1.5">
              <span className="w-2 h-2 rounded-full bg-rose-500" />
              STANDALONE / DISCONNECTED
            </span>
          )}
        </div>
      </div>

      {/* Grid of System Components */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* 1. Gateway & Host Configuration */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-3xl p-6 shadow-xl space-y-4">
          <div className="flex items-center space-x-2.5 text-sm font-bold text-white border-b border-slate-800 pb-3">
            <Network className="w-4 h-4 text-sky-400" />
            <span>ネットワーク通信 ＆ ゲートウェイ設定</span>
          </div>

          <div className="space-y-3 text-xs">
            <div>
              <label className="block text-slate-400 mb-1 font-medium">
                ATOMS3 Lite ゲートウェイ ホスト名 / IPアドレス:
              </label>
              <div className="flex items-center gap-2">
                <input
                  type="text"
                  value={gatewayHost}
                  onChange={(e) => setGatewayHost(e.target.value)}
                  placeholder="plant-doctor.local"
                  className="flex-1 bg-slate-950 text-slate-200 px-3 py-2 rounded-xl border border-slate-700/80 font-mono text-xs outline-none focus:border-emerald-500"
                />
              </div>
              <p className="text-[11px] text-slate-500 mt-1">
                同一 Wi-Fi 内の mDNS 名（<code>plant-doctor.local</code>）または固定IPを指定
              </p>
            </div>

            <div className="pt-2 border-t border-slate-800/80 flex items-center justify-between text-[11px] font-mono text-slate-400">
              <span>最新テレメトリSeq:</span>
              <span className="text-white font-bold">#{seq}</span>
            </div>
            <div className="flex items-center justify-between text-[11px] font-mono text-slate-400">
              <span>デバイスRTC時刻:</span>
              <span className="text-white font-bold">{timeFormatted}</span>
            </div>
            <div className="flex items-center justify-between text-[11px] font-mono text-slate-400">
              <span>ポンプ駆動ステータス:</span>
              <span className={pumpOn ? 'text-sky-400 font-bold' : 'text-slate-400 font-bold'}>
                {pumpOn ? '駆動中 (ACTIVE)' : '待機中 (STANDBY)'}
              </span>
            </div>
          </div>

          {/* Sync RTC Button */}
          <button
            onClick={onSyncTime}
            className="w-full mt-2 flex items-center justify-center space-x-2 bg-gradient-to-r from-emerald-600 to-teal-600 hover:from-emerald-500 hover:to-teal-500 text-white font-bold py-2.5 px-4 rounded-xl shadow-lg shadow-emerald-600/20 text-xs transition active:scale-95 cursor-pointer"
          >
            <Clock className="w-4 h-4" />
            <span>PCの現在時刻をマイコンRTCへ同期</span>
          </button>
        </div>

        {/* 2. Hardware Safety Interlocks */}
        <div className="bg-slate-900/80 border border-slate-800/80 rounded-3xl p-6 shadow-xl space-y-4">
          <div className="flex items-center space-x-2.5 text-sm font-bold text-white border-b border-slate-800 pb-3">
            <ShieldCheck className="w-4 h-4 text-emerald-400" />
            <span>ハードウェア安全インターロック機構</span>
          </div>

          <div className="space-y-3 text-xs text-slate-300">
            <div className="flex items-start gap-2.5 p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <CheckCircle2 className="w-4 h-4 text-emerald-400 mt-0.5 flex-shrink-0" />
              <div>
                <div className="font-bold text-white">給水ポンプ最大駆動時間リミッタ (2.0秒)</div>
                <div className="text-[11px] text-slate-400">
                  マイコン内蔵タイマにより、いかなるソフトウェアバグでも2秒で強制遮断
                </div>
              </div>
            </div>

            <div className="flex items-start gap-2.5 p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <CheckCircle2 className="w-4 h-4 text-emerald-400 mt-0.5 flex-shrink-0" />
              <div>
                <div className="font-bold text-white">クールダウン インターバル保護 (3.0秒)</div>
                <div className="text-[11px] text-slate-400">
                  給水動作後の連続空打ちをハードウェア・ステートマシンが物理的に拒絶
                </div>
              </div>
            </div>

            <div className="flex items-start gap-2.5 p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              {tankLiquid ? (
                <CheckCircle2 className="w-4 h-4 text-emerald-400 mt-0.5 flex-shrink-0" />
              ) : (
                <AlertTriangle className="w-4 h-4 text-rose-400 mt-0.5 flex-shrink-0" />
              )}
              <div>
                <div className="font-bold text-white">光電式液面センサによる空焚き保護</div>
                <div className="text-[11px] text-slate-400">
                  水タンク渇水時はポンプ通電を自動遮断し、モーターの空転破損を防止
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* 3. Sensor Fleet Specifications */}
        <div className="md:col-span-2 bg-slate-900/80 border border-slate-800/80 rounded-3xl p-6 shadow-xl space-y-4">
          <div className="flex items-center space-x-2.5 text-sm font-bold text-white border-b border-slate-800 pb-3">
            <Zap className="w-4 h-4 text-amber-400" />
            <span>搭載センサ・アクチュエータ一覧</span>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3 text-xs">
            <div className="p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <div className="font-bold text-white flex items-center gap-1.5">
                <span className="w-2 h-2 rounded-full bg-emerald-400" />
                SEN0206
              </div>
              <div className="text-slate-400 text-[11px] mt-0.5">非接触赤外線葉温センサ (I2C)</div>
              <div className="text-slate-500 text-[10px] mt-1 font-mono">生体表面放射率 0.98 補正</div>
            </div>

            <div className="p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <div className="font-bold text-white flex items-center gap-1.5">
                <span className="w-2 h-2 rounded-full bg-sky-400" />
                BME280
              </div>
              <div className="text-slate-400 text-[11px] mt-0.5">精密気温・湿度センサ (I2C)</div>
              <div className="text-slate-500 text-[10px] mt-1 font-mono">±0.5℃, ±3%RH 精度</div>
            </div>

            <div className="p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <div className="font-bold text-white flex items-center gap-1.5">
                <span className="w-2 h-2 rounded-full bg-amber-400" />
                SEN0193
              </div>
              <div className="text-slate-400 text-[11px] mt-0.5">静電容量式土壌水分センサ (ADC)</div>
              <div className="text-slate-500 text-[10px] mt-1 font-mono">非腐食・高耐久プローブ</div>
            </div>

            <div className="p-3 rounded-2xl bg-slate-950/60 border border-slate-800">
              <div className="font-bold text-white flex items-center gap-1.5">
                <span className="w-2 h-2 rounded-full bg-yellow-400" />
                SEN0228
              </div>
              <div className="text-slate-400 text-[11px] mt-0.5">高精度環境光センサ (ADC)</div>
              <div className="text-slate-500 text-[10px] mt-1 font-mono">人間視感度・光合成波長域</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
