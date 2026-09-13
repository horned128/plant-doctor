# Plant Medical - PC リアルタイム監視ステーション

DT-EBML63Q2557（ROHM ML63Q2557 / Solist-AI™）で測定・診断された植物生体・環境データを、ATOMS3 Lite ゲートウェイを介して LAN 内の PC でリアルタイムにモニタリング・可視化・分析するための Web ダッシュボードおよび分析ツール群です。

## 主な機能

- **植物ストレスバロメータ (0〜100)**: 円形ビジュアルメーターと動的ステータスラベル
- **AI 総合診断カード**: `HEALTHY`, `HEAT_STRESS`, `DRY_STRESS`, `WATERING`, `WATERING_FAILED` 等の詳細状態と土壌トレンド
- **環境 & 生体センサ計測値**: 葉温、気温、湿度、照度、土壌水分Raw値、および蒸散指標となる葉温&minus;気温差（&Delta;T）
- **リアルタイム時系列グラフ**: 気温 vs 葉温の推移、土壌水分推移
- **給水制御 & 安全インターロック**: 水タンク残量表示、ポンプ動作状態、遠隔手動給水テスト、デモモード切替
- **CSV エクスポート**: ブラウザ上で蓄積された時系列測定データをワンクリックで CSV ダウンロード
- **RTC 時刻自動同期**: PC の現在時刻をワンクリックでマイコン側の RTC と同期

---

## 起動方法

### 方法 1: ゼロインストール（最も簡単）

ATOMS3 Lite 内蔵の Web サーバから直接ダッシュボードが配信されます。
同一 Wi-Fi に接続された PC やスマートフォンのブラウザで以下を開くだけです：

```text
http://plant-doctor.local
```

---

### 方法 2: PC 上での専用開発・実行 (Node.js / Vite)

PC 側で長時間のロギングを行ったり、UI をカスタマイズする場合：

```bash
cd software/plant-medical

# 依存パッケージのインストール
npm install

# 開発サーバ起動 (ポート 3000)
npm run dev
```

ブラウザで `http://localhost:3000` を開きます。
画面上部の「Host:」入力欄に ATOMS3 Lite のホスト名（`plant-doctor.local`）または IP アドレスを入力すると、リアルタイムに接続されます。

---

### 方法 3: Python CLI & CSV ロガー

Python 3 がインストールされた PC で直接バックグラウンドロギングを行う場合：

```bash
# リアルタイム表示
python scripts/plant_medical_cli.py

# CSVファイルへ連続保存
python scripts/plant_medical_cli.py --log plant_log_2026.csv --interval 1.0

# 遠隔給水テストの実行
python scripts/plant_medical_cli.py --water
```
