# ATOMS3 Lite (ESP32-S3) Gateway for Plant Doctor

DT-EBML63Q2557（ROHM ML63Q2557）の UART 通信ポート（FTDI FT2232H）と USB 接続し、取得したセンサ値やエッジAI診断結果を Wi-Fi / LAN 経由で PC やスマートフォンへ配信するゲートウェイファームウェアです。

## 主な機能

- **USB Host (FTDI FT2232H)**: DT-EBML63Q2557 の `CN9` (Type-C) と USB-C ケーブル 1 本で接続し、115,200 bps で通信
- **テレメトリ自動ポーリング**: 1 秒周期で `S` (センサ値), `Q` (AI診断結果), `W?` (ポンプ状態) を自動取得
- **Wi-Fi & mDNS**: ローカル LAN に接続し、`http://plant-doctor.local` でアクセス可能
- **WebSocket & REST API**: JSON 形式でリアルタイムストリーミング
- **内蔵 Web ダッシュボード**: ブラウザを開くだけで即座にストレスバロメータや環境推移グラフをモニタリング
- **遠隔操作**: Web 上または ATOMS3 Lite 本体ボタン（GPIO 41）から給水テスト、デモモード切替、RTC 時刻同期が可能
- **LED ステータスインジケータ**: 内蔵 RGB LED (WS2812B / GPIO 35) で接続・給水状態を可視化

---

## ハードウェア接続

1. **USB 接続（本命案）**:
   - DT-EBML63Q2557: `CN9` (USB 通信 Type-C コネクタ)
   - ATOMS3 Lite: `Type-C` ポート
   - USB-C to USB-C ケーブルで直接接続します。
   - ※ ATOMS3 Lite は USB Host モードで動作します。

2. **電源供給**:
   - DT-EBML63Q2557: `CN8` (USB 電源 Type-C) または `CN7` (乾電池) より給電
   - ATOMS3 Lite: 底面ピンヘッダ（5V/GND）または Grove 端子（5V/GND）より 5V 給電

3. **UART 直結フォールバック（予備案）**:
   - ATOMS3 Lite 背面 Grove ポート (G1: TX, G2: RX, 5V, GND) を使用して、直接 3.3V UART でシリアル通信することも可能です。

---

## ビルド & 書き込み手順 (ESP-IDF)

標準の ESP-IDF v5.x 環境でビルドできます。

```bash
cd firmware/esp32s3

# 1. Wi-Fi SSID / パスワードの設定
# main/app_config.h を編集するか、menuconfig で設定
idf.py set-target esp32s3
idf.py menuconfig

# 2. ビルド
idf.py build

# 3. ATOMS3 Lite へ書き込み & モニタリング
idf.py -p COMx flash monitor
```

---

## 使い方

1. ATOMS3 Lite の起動後、Wi-Fi に接続されると RGB LED が **緑色** に点灯します。
2. 同一 LAN 内の PC やスマートフォンのブラウザで以下のアドレスを開きます：
   ```text
   http://plant-doctor.local
   ```
   （または ATOMS3 Lite の IP アドレス）
3. リアルタイムに植物ストレス度（0〜100）や葉温、土壌水分、AI 診断結果が表示されます。
