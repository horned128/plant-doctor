import { PlantProfile } from '../types';

const STORAGE_KEY_PROFILES = 'plant_doctor_profiles_v1';
const STORAGE_KEY_ACTIVE_ID = 'plant_doctor_active_profile_id_v1';

export const DEFAULT_PROFILES: PlantProfile[] = [
  {
    id: 'pothos-default',
    name: 'オフィスのポトス',
    species: 'pothos',
    speciesNameJa: 'ポトス (黄金葛)',
    description: '耐陰性が高く乾燥に強いサトイモ科植物。水のやりすぎによる根腐れに注意が必要。',
    thresholds: {
      soilMinRaw: 2200,      // 乾き気味でも耐える
      soilMaxRaw: 1650,
      tempMin: 15.0,
      tempMax: 30.0,
      luxMin: 150,
      luxMax: 800,
      wiltSensitivity: 40,   // しおれ耐性高め
    },
    baselineFeatures: [50, 50, 50, 48, 52, 50, 50, 50],
    baselineLoss: 0.0210,
    learnedSamples: 64,
    updatedAt: Date.now() - 86400000 * 3,
  },
  {
    id: 'spathiphyllum-default',
    name: 'リビングのスパティフラム',
    species: 'spathiphyllum',
    speciesNameJa: 'スパティフラム (笹笹)',
    description: '湿潤な土壌と高い湿度を好む。水切れすると葉が顕著にしおれて垂れ下がるため早期給水が肝心。',
    thresholds: {
      soilMinRaw: 2050,      // 早めに水やりを要求
      soilMaxRaw: 1500,
      tempMin: 18.0,
      tempMax: 28.0,
      luxMin: 200,
      luxMax: 750,
      wiltSensitivity: 85,   // しおれに極めて敏感
    },
    baselineFeatures: [65, 50, 50, 42, 60, 50, 50, 50],
    baselineLoss: 0.0185,
    learnedSamples: 98,
    updatedAt: Date.now() - 86400000 * 2,
  },
  {
    id: 'custom-default',
    name: 'マイグリーン (観葉植物)',
    species: 'custom',
    speciesNameJa: 'その他の植物 (カスタム)',
    description: '汎用観葉植物モデル。ユーザーの植物の設置環境と生体データに合わせてパーソナル学習・適応。',
    thresholds: {
      soilMinRaw: 2100,
      soilMaxRaw: 1600,
      tempMin: 16.0,
      tempMax: 28.0,
      luxMin: 200,
      luxMax: 1000,
      wiltSensitivity: 50,
    },
    baselineFeatures: [55, 50, 50, 45, 55, 50, 50, 50],
    baselineLoss: 0.0245,
    learnedSamples: 42,
    updatedAt: Date.now() - 86400000,
  },
];

export function loadPlantProfiles(): PlantProfile[] {
  try {
    const raw = localStorage.getItem(STORAGE_KEY_PROFILES);
    if (raw) {
      const parsed = JSON.parse(raw);
      if (Array.isArray(parsed) && parsed.length > 0) {
        return parsed;
      }
    }
  } catch (e) {
    console.warn('Failed to load plant profiles from localStorage:', e);
  }
  return DEFAULT_PROFILES;
}

export function savePlantProfiles(profiles: PlantProfile[]): void {
  try {
    localStorage.setItem(STORAGE_KEY_PROFILES, JSON.stringify(profiles));
  } catch (e) {
    console.error('Failed to save plant profiles:', e);
  }
}

export function loadActiveProfileId(): string {
  try {
    const id = localStorage.getItem(STORAGE_KEY_ACTIVE_ID);
    if (id) return id;
  } catch (e) {
    // ignore
  }
  return DEFAULT_PROFILES[0].id;
}

export function saveActiveProfileId(id: string): void {
  try {
    localStorage.setItem(STORAGE_KEY_ACTIVE_ID, id);
  } catch (e) {
    console.error(e);
  }
}

/**
 * 安定稼働時の現在センサー値から、該当植物のパーソナル生体基準モデルを更新
 */
export function updateProfileWithPersonalLearning(
  profile: PlantProfile,
  currentFeatures: number[],
  currentLoss: number
): PlantProfile {
  // 過去の基準と現在の安定値を指数移動平均（EMA）でブレンド
  const alpha = 0.25; // 25% current, 75% historical
  const updatedFeatures = profile.baselineFeatures.map((base, idx) => {
    const cur = currentFeatures[idx] ?? base;
    return Math.round(base * (1 - alpha) + cur * alpha);
  });

  const updatedLoss = Number((profile.baselineLoss * (1 - alpha) + currentLoss * alpha).toFixed(4));

  const updated: PlantProfile = {
    ...profile,
    baselineFeatures: updatedFeatures,
    baselineLoss: updatedLoss,
    learnedSamples: profile.learnedSamples + 1,
    updatedAt: Date.now(),
  };

  return updated;
}
