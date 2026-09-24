import React, { useEffect, useRef, useState } from 'react';
import { Sparkles, Droplets, Sun, AlertTriangle, CheckCircle2, Waves, Activity } from 'lucide-react';

interface DigitalPlantTwinProps {
  status: string;        // 'HEALTHY' | 'HEAT_STRESS' | 'DRY_STRESS' | 'WATERING' | 'WATERING_FAILED' etc.
  stress: number;        // 0 to 100
  pumpOn?: boolean;
  soilRaw?: number;
  leafTemp?: number;
  leafAirDiff?: number;
  lux?: number;
  interactive?: boolean;
  onPlantClick?: () => void;
}

interface Particle {
  x: number;
  y: number;
  vx: number;
  vy: number;
  size: number;
  alpha: number;
  life: number;
  maxLife: number;
  color: string;
}

export const DigitalPlantTwin: React.FC<DigitalPlantTwinProps> = ({
  status,
  stress,
  pumpOn = false,
  soilRaw = 1800,
  leafTemp = 24.0,
  leafAirDiff = -0.5,
  lux = 450,
  interactive = true,
  onPlantClick,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [touchBounce, setTouchBounce] = useState(false);
  const [isHovered, setIsHovered] = useState(false);

  const isWatering = status === 'WATERING' || pumpOn;
  const isDry = status === 'DRY_STRESS' || stress >= 50;
  const isHeat = status === 'HEAT_STRESS';
  const isAlert = status === 'WATERING_FAILED' || status === 'SOIL_SENSOR_ERROR';
  const isHealthy = !isWatering && !isDry && !isHeat && !isAlert && stress < 30;

  // Handle click / tap spring effect
  const handleInteraction = () => {
    if (!interactive) return;
    setTouchBounce(true);
    setTimeout(() => setTouchBounce(false), 700);
    if (onPlantClick) onPlantClick();
  };

  // Canvas particle simulation (Water droplets, Bio-energy photons, Heat haze)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animId: number;
    let particles: Particle[] = [];
    const width = (canvas.width = canvas.offsetWidth * (window.devicePixelRatio || 1) || 400);
    const height = (canvas.height = canvas.offsetHeight * (window.devicePixelRatio || 1) || 440);

    const spawnParticle = () => {
      if (isWatering) {
        // Hydrating raindrops falling onto pot
        for (let i = 0; i < 3; i++) {
          particles.push({
            x: Math.random() * (width * 0.6) + width * 0.2,
            y: 0,
            vx: (Math.random() - 0.5) * 0.4,
            vy: Math.random() * 3.5 + 4.5,
            size: Math.random() * 2.5 + 1.5,
            alpha: 0.85,
            life: 0,
            maxLife: 60,
            color: '#38bdf8', // sky-400
          });
        }
      } else if (isHealthy) {
        // Floating bio-photons / vitality sparks
        if (Math.random() < 0.35) {
          particles.push({
            x: Math.random() * (width * 0.6) + width * 0.2,
            y: height * 0.72,
            vx: (Math.random() - 0.5) * 0.7,
            vy: -(Math.random() * 1.2 + 0.4),
            size: Math.random() * 2.8 + 1.2,
            alpha: 0.75,
            life: 0,
            maxLife: 75,
            color: '#34d399', // emerald-400
          });
        }
      } else if (isHeat) {
        // Rising heat shimmer waves
        if (Math.random() < 0.4) {
          particles.push({
            x: Math.random() * (width * 0.7) + width * 0.15,
            y: height * 0.75,
            vx: Math.sin(Date.now() / 200) * 1.0,
            vy: -(Math.random() * 2 + 1),
            size: Math.random() * 3.5 + 1.5,
            alpha: 0.5,
            life: 0,
            maxLife: 55,
            color: '#fb923c', // orange-400
          });
        }
      }
    };

    const render = () => {
      ctx.clearRect(0, 0, width, height);
      spawnParticle();

      for (let i = particles.length - 1; i >= 0; i--) {
        const p = particles[i];
        p.life++;
        p.x += p.vx;
        p.y += p.vy;

        const lifeRatio = p.life / p.maxLife;
        const currentAlpha = p.alpha * (1 - lifeRatio);

        if (lifeRatio >= 1 || p.y > height * 0.82) {
          // Splash ripple on ground for raindrops
          if (isWatering && p.y > height * 0.76) {
            ctx.strokeStyle = `rgba(56, 189, 248, ${Math.max(0, currentAlpha * 0.6)})`;
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            ctx.ellipse(p.x, height * 0.78, (1 - lifeRatio) * 14, (1 - lifeRatio) * 4, 0, 0, Math.PI * 2);
            ctx.stroke();
          }
          particles.splice(i, 1);
          continue;
        }

        ctx.fillStyle = p.color;
        ctx.globalAlpha = Math.max(0, currentAlpha);
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
        ctx.fill();
      }

      ctx.globalAlpha = 1.0;
      animId = requestAnimationFrame(render);
    };

    render();

    return () => {
      cancelAnimationFrame(animId);
    };
  }, [isWatering, isHealthy, isHeat, isDry]);

  // Leaf angles based on stress & watering states
  let leafAngle1 = -38; // Left upper leaf
  let leafAngle2 = 36;  // Right upper leaf
  let leafAngle3 = -62; // Left lower leaf
  let leafAngle4 = 58;  // Right lower leaf

  let leafColorPrimary = '#10b981'; // Healthy vibrant emerald
  let leafColorShade = '#047857';
  let potSoilColor = '#2d2217'; // Moist rich soil
  let auraGlow = 'rgba(16, 185, 129, 0.22)';
  let glowVar = 'rgba(16, 185, 129, 0.28)';

  if (isWatering) {
    leafAngle1 = -32;
    leafAngle2 = 30;
    leafAngle3 = -55;
    leafAngle4 = 50;
    leafColorPrimary = '#059669';
    leafColorShade = '#064e3b';
    potSoilColor = '#172554'; // Deep hydrated
    auraGlow = 'rgba(56, 189, 248, 0.35)';
    glowVar = 'rgba(56, 189, 248, 0.4)';
  } else if (isDry) {
    // Wilted & drooping leaves
    leafAngle1 = -18;
    leafAngle2 = 18;
    leafAngle3 = -20;
    leafAngle4 = 22;
    leafColorPrimary = '#84cc16'; // Pale dry olive
    leafColorShade = '#4d7c0f';
    potSoilColor = '#453728'; // Parched soil
    auraGlow = 'rgba(234, 179, 8, 0.18)';
    glowVar = 'rgba(234, 179, 8, 0.25)';
  } else if (isHeat) {
    // Stressed from heat
    leafAngle1 = -24;
    leafAngle2 = 22;
    leafAngle3 = -28;
    leafAngle4 = 30;
    leafColorPrimary = '#eab308'; // Stressed amber
    leafColorShade = '#a16207';
    potSoilColor = '#382e25';
    auraGlow = 'rgba(249, 115, 22, 0.28)';
    glowVar = 'rgba(249, 115, 22, 0.35)';
  } else if (isAlert) {
    leafColorPrimary = '#f43f5e';
    leafColorShade = '#9f1239';
    auraGlow = 'rgba(244, 63, 94, 0.32)';
    glowVar = 'rgba(244, 63, 94, 0.4)';
  }

  return (
    <div
      onClick={handleInteraction}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
      className="relative w-full h-full min-h-[380px] lg:min-h-[440px] rounded-3xl bg-slate-950/40 border border-slate-800/60 flex flex-col items-center justify-between p-4 overflow-hidden select-none cursor-pointer transition-all duration-700 backdrop-blur-md group"
      style={
        {
          '--glow-color': glowVar,
          boxShadow: `0 0 60px ${auraGlow}, inset 0 0 40px rgba(0,0,0,0.6)`,
        } as React.CSSProperties
      }
    >
      {/* Background Ambient Aura Glow */}
      <div
        className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-72 h-72 rounded-full blur-3xl pointer-events-none transition-all duration-1000 opacity-70"
        style={{ backgroundColor: auraGlow }}
      />

      {/* Top Clinical Status HUD Capsule */}
      <div className="w-full flex items-center justify-between z-10">
        <div className="flex items-center space-x-2">
          {isWatering ? (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-full text-xs font-bold bg-sky-500/20 text-sky-400 border border-sky-500/50 animate-pulse shadow-lg shadow-sky-500/20">
              <Droplets className="w-3.5 h-3.5 animate-bounce" />
              <span>給水中 (Watering Active)</span>
            </div>
          ) : isDry ? (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-full text-xs font-bold bg-amber-500/20 text-amber-400 border border-amber-500/50 shadow-lg shadow-amber-500/20">
              <AlertTriangle className="w-3.5 h-3.5" />
              <span>乾燥ストレス (Dry Stress)</span>
            </div>
          ) : isHeat ? (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-full text-xs font-bold bg-orange-500/20 text-orange-400 border border-orange-500/50 animate-pulse shadow-lg shadow-orange-500/20">
              <Sun className="w-3.5 h-3.5" />
              <span>熱ストレス (Heat Stress)</span>
            </div>
          ) : isAlert ? (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-full text-xs font-bold bg-rose-500/20 text-rose-400 border border-rose-500/50 animate-pulse shadow-lg shadow-rose-500/20">
              <AlertTriangle className="w-3.5 h-3.5" />
              <span>生体・給水警告</span>
            </div>
          ) : (
            <div className="flex items-center space-x-1.5 px-3 py-1 rounded-full text-xs font-bold bg-emerald-500/20 text-emerald-300 border border-emerald-500/50 shadow-lg shadow-emerald-500/10">
              <CheckCircle2 className="w-3.5 h-3.5" />
              <span>平常健常 (Healthy Twin)</span>
              <span>平常健常 (良好)</span>
            </div>
          )}
        </div>

        {/* Dynamic Bio-Signal Pulse Icon */}
        <div className="flex items-center space-x-2 text-[11px] text-slate-300 font-mono bg-slate-900/80 px-3 py-1 rounded-full border border-slate-700/60 shadow">
          <Sparkles className={`w-3.5 h-3.5 ${isHealthy ? 'text-emerald-400 animate-pulse' : 'text-slate-500'}`} />
          <span>
            {isWatering
              ? '水分浸透中'
              : isHealthy
              ? '蒸散冷却中 (ΔT健全)'
              : isDry
              ? '気孔閉鎖・水分保持'
              : '熱放散限界'}
          </span>
        </div>
      </div>

      {/* Particle Overlay Canvas */}
      <canvas
        ref={canvasRef}
        className="absolute inset-0 w-full h-full pointer-events-none z-10"
      />

      {/* Central Botanical Digital Twin (Large & Breathing) */}
      <div
        className={`relative w-72 sm:w-80 h-64 sm:h-72 flex items-center justify-center transition-transform duration-500 ${
          touchBounce ? 'scale-105 -rotate-1' : isHovered ? 'scale-[1.03]' : 'scale-100'
        } animate-bio-breathe`}
      >
        {/* Floating HUD Annotations linked to anatomy */}
        {/* Canopy HUD (Top right: Lux) */}
        <div className="absolute top-2 right-4 pointer-events-none hidden sm:flex items-center gap-1.5 text-[10px] font-mono text-slate-400 bg-slate-900/60 px-2 py-0.5 rounded border border-slate-800 backdrop-blur-xs">
          <Sun className="w-3 h-3 text-amber-400" />
          <span>{lux} lx</span>
        </div>

        {/* Leaf HUD (Left middle: Leaf Temp & ΔT) */}
        <div className="absolute top-1/3 left-1 pointer-events-none hidden sm:flex flex-col gap-0.5 text-[10px] font-mono bg-slate-900/70 p-1.5 rounded-lg border border-slate-800 backdrop-blur-xs shadow-md">
          <div className="flex items-center gap-1 text-emerald-400 font-semibold">
            <Activity className="w-3 h-3" />
            <span>葉温 {leafTemp.toFixed(1)}℃</span>
          </div>
          <div className="text-[9px] text-slate-400">
            ΔT: <span className={leafAirDiff < 0 ? 'text-emerald-300' : 'text-amber-400'}>{leafAirDiff > 0 ? '+' : ''}{leafAirDiff.toFixed(2)}℃</span>
          </div>
        </div>

        {/* SVG Living Plant */}
        <svg
          viewBox="0 0 240 220"
          className="w-full h-full filter drop-shadow-[0_12px_24px_rgba(0,0,0,0.7)]"
        >
          <defs>
            {/* Ceramic Pot Gradient */}
            <linearGradient id="potGradTwin" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#334155" />
              <stop offset="40%" stopColor="#1e293b" />
              <stop offset="100%" stopColor="#0f172a" />
            </linearGradient>

            {/* Pot Rim Highlight */}
            <linearGradient id="rimGradTwin" x1="0%" y1="0%" x2="100%" y2="0%">
              <stop offset="0%" stopColor="#64748b" />
              <stop offset="50%" stopColor="#94a3b8" />
              <stop offset="100%" stopColor="#475569" />
            </linearGradient>

            {/* Leaf Color Gradient (Organic Vein & Shade) */}
            <linearGradient id="leafGradTwin" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor={leafColorPrimary} stopOpacity="1" />
              <stop offset="100%" stopColor={leafColorShade} stopOpacity="0.88" />
            </linearGradient>

            {/* Light Sheen on leaves */}
            <linearGradient id="sheenTwin" x1="0%" y1="0%" x2="50%" y2="100%">
              <stop offset="0%" stopColor="#ffffff" stopOpacity="0.38" />
              <stop offset="100%" stopColor="#ffffff" stopOpacity="0" />
            </linearGradient>
          </defs>

          {/* === STEM & BRANCHES === */}
          <g className="transition-all duration-700 ease-out">
            {/* Main Central Stem */}
            <path
              d="M 120 170 Q 120 120 120 70"
              stroke="#047857"
              strokeWidth="5.5"
              strokeLinecap="round"
              fill="none"
            />
            {/* Left Branch */}
            <path
              d="M 120 125 Q 100 110 75 105"
              stroke="#047857"
              strokeWidth="4.5"
              strokeLinecap="round"
              fill="none"
            />
            {/* Right Branch */}
            <path
              d="M 120 115 Q 140 100 165 95"
              stroke="#047857"
              strokeWidth="4.5"
              strokeLinecap="round"
              fill="none"
            />
          </g>

          {/* === LEAF 1: Left Lower (L3) === */}
          <g
            transform={`translate(75, 105) rotate(${leafAngle3})`}
            className="transition-transform duration-700 ease-out"
          >
            <path
              d="M 0 0 C -25 -20 -45 -10 -55 10 C -45 35 -15 25 0 0 Z"
              fill="url(#leafGradTwin)"
            />
            <path
              d="M 0 0 C -25 -20 -45 -10 -55 10"
              stroke="url(#sheenTwin)"
              strokeWidth="1.5"
              fill="none"
            />
            <path d="M 0 0 Q -28 5 -50 9" stroke="#064e3b" strokeWidth="1.2" fill="none" />
          </g>

          {/* === LEAF 2: Right Lower (L4) === */}
          <g
            transform={`translate(165, 95) rotate(${leafAngle4})`}
            className="transition-transform duration-700 ease-out"
          >
            <path
              d="M 0 0 C 25 -20 45 -10 55 10 C 45 35 15 25 0 0 Z"
              fill="url(#leafGradTwin)"
            />
            <path
              d="M 0 0 C 25 -20 45 -10 55 10"
              stroke="url(#sheenTwin)"
              strokeWidth="1.5"
              fill="none"
            />
            <path d="M 0 0 Q 28 5 50 9" stroke="#064e3b" strokeWidth="1.2" fill="none" />
          </g>

          {/* === LEAF 3: Left Upper (L1) === */}
          <g
            transform={`translate(100, 85) rotate(${leafAngle1})`}
            className="transition-transform duration-700 ease-out"
          >
            <path
              d="M 0 0 C -20 -30 -40 -35 -55 -15 C -50 15 -20 15 0 0 Z"
              fill="url(#leafGradTwin)"
            />
            <path
              d="M 0 0 C -20 -30 -40 -35 -55 -15"
              stroke="url(#sheenTwin)"
              strokeWidth="1.8"
              fill="none"
            />
            <path d="M 0 0 Q -30 -12 -50 -14" stroke="#064e3b" strokeWidth="1.2" fill="none" />
          </g>

          {/* === LEAF 4: Right Upper (L2) === */}
          <g
            transform={`translate(140, 75) rotate(${leafAngle2})`}
            className="transition-transform duration-700 ease-out"
          >
            <path
              d="M 0 0 C 20 -30 40 -35 55 -15 C 50 15 20 15 0 0 Z"
              fill="url(#leafGradTwin)"
            />
            <path
              d="M 0 0 C 20 -30 40 -35 55 -15"
              stroke="url(#sheenTwin)"
              strokeWidth="1.8"
              fill="none"
            />
            <path d="M 0 0 Q 30 -12 50 -14" stroke="#064e3b" strokeWidth="1.2" fill="none" />
          </g>

          {/* === LEAF 5: Crown Top Sprout === */}
          <g
            transform={`translate(120, 68) rotate(${isDry ? 14 : 0})`}
            className="transition-transform duration-700 ease-out"
          >
            <path
              d="M 0 0 C -12 -28 0 -45 6 -42 C 12 -38 18 -18 0 0 Z"
              fill="url(#leafGradTwin)"
            />
            <path d="M 0 0 Q 3 -25 5 -40" stroke="url(#sheenTwin)" strokeWidth="1.5" fill="none" />
          </g>

          {/* === CERAMIC POT & SOIL === */}
          {/* Soil Bed */}
          <ellipse
            cx="120"
            cy="168"
            rx="46"
            ry="11"
            fill={potSoilColor}
            className="transition-colors duration-700"
          />

          {/* Pot Base Body */}
          <path
            d="M 74 168 L 86 210 Q 120 216 154 210 L 166 168 Z"
            fill="url(#potGradTwin)"
            stroke="#475569"
            strokeWidth="1.5"
          />

          {/* Pot Rim */}
          <ellipse
            cx="120"
            cy="168"
            rx="48"
            ry="9"
            fill="url(#rimGradTwin)"
            stroke="#64748b"
            strokeWidth="1.2"
          />

          {/* Pot Logo / Solist-AI Plant Doctor Crest */}
          <circle cx="120" cy="192" r="6" fill="#0f172a" stroke="#10b981" strokeWidth="1" />
          <path d="M 120 189 L 120 195 M 117 192 L 123 192" stroke="#10b981" strokeWidth="1.2" strokeLinecap="round" />
        </svg>
      </div>

      {/* Bottom Subtitle / Micro Indicator */}
      <div className="w-full flex items-center justify-between text-[11px] text-slate-400 pt-2 border-t border-slate-800/80 z-10">
        <span className="flex items-center gap-1.5 font-medium text-slate-300">
          <Waves className="w-3.5 h-3.5 text-teal-400" />
          <span>植物生体デジタルツイン (Digital Twin)</span>
          <span>植物の生体状態 (生命バイオリズム)</span>
        </span>
        <span className="text-slate-500 font-mono text-[10px]">
          タップで反応 &bull; 土壌Raw: {soilRaw}
        </span>
      </div>
    </div>
  );
};
