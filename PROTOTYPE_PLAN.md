このディレクトリを、DT-EBML63Q2557を使用した「Plant Doctor」ファームウェアのGitリポジトリとして整理し、最初のBring-up版を作成してください。

# 1. 使用環境

対象ボード：
- データ・テクノ DT-EBML63Q2557
- MCU：ROHM ML63Q2557
- CPU：Arm Cortex-M0+
- AIアクセラレータ：AxlCORE-ODL

現在の接続：
- DT-EBML63Q2557 ⇔ PC：USB Type-Cによる給電
- DT-EBML63Q2557 ⇔ MCU-Link：SWDデバッグ接続
- MCU-Link ⇔ PC：USB接続
- FTDI、COMポート、MtProxy、Solist-AI Scope、Hostアプリは現段階では使用しない

開発環境：
- LEXIDE-Ω
- MCU-Linkによる書き込み・デバッグ
- C言語
- ベアメタル
- スーパーループ＋状態機械を基本とする
- 動的メモリ確保は禁止

# 2. Plant Doctorの最終目的

Plant Doctorは、室内植物の状態を複数センサで監視し、植物ごとの通常状態をSolist-AIでオンデバイス学習し、異常検知と自動給水を行うスタンドアローン装置である。

クラウド接続を必須とせず、ML63Q2557単体で以下を実行する。

- センサデータ取得
- 特徴量生成
- 正常状態の初期学習
- 通常監視
- 異常度算出
- 給水判断
- ポンプ制御
- 給水後の土壌応答評価
- LCDとLEDによる状態表示
- ログ保存

# 3. 想定する入力

最終的には次のセンサを扱う予定である。

必須候補：
- アナログ土壌水分センサ
- 葉温を測定する赤外線温度センサ
- 気温・湿度センサ
- 照度センサ

任意・将来拡張：
- 付属MEMS加速度センサ
- ポンプの振動監視
- 水タンク残量
- 電源監視

センサ型式や接続ピンが未確定のものについては、具体的なドライバを仮定せず、インターフェースとスタブを用意すること。

# 4. 想定する出力

- 基板上LCD
- 基板上LED
- 基板上スイッチ
- 給水ポンプ
- 必要に応じてリレーまたはGPIO出力
- FRAM等へのログ保存

# 5. Plant Doctorが検出する異常

将来的に以下を検出する。

植物状態：
- 葉温と気温の差が通常と異なる
- 蒸散リズムが通常と異なる
- 土壌水分の低下速度が通常と異なる
- 同一照度・湿度条件での反応が通常と異なる

給水系：
- ポンプを動かしても土壌水分が上昇しない
- ホース抜け
- 水切れ
- ポンプ異常
- 土壌の吸水応答が通常と異なる
- センサ故障または断線

最初のAI対象としては、「給水後の土壌水分応答の異常検知」を優先する。

# 6. AI入力として将来使用する特徴量

加速度異常検知サンプルのFFT設定をそのまま流用しないこと。

植物向けでは、将来的に次の特徴量を使用する。

- 土壌水分
- 土壌水分の移動平均
- 土壌水分の変化速度
- 給水前後の水分差
- 給水後の最大増加量
- 給水後の安定化時間
- 葉温
- 気温
- 葉温－気温
- 湿度
- 照度
- 前回給水からの経過時間
- 時刻

今回のBring-up版ではAI処理を実装しなくてよいが、後でSolist-AIのMlTask、Preprocess、AnomalyDetectorを追加できる構造にすること。

# 7. アプリケーション状態

将来的に次の状態を持つ。

- BOOT
- SELF_TEST
- SENSOR_WARMUP
- BASELINE_LEARN
- MONITOR
- WATERING
- RESPONSE_EVALUATION
- ALERT
- SENSOR_ERROR

今回実装する状態は以下まででよい。

- BOOT
- SELF_TEST
- MONITOR
- ERROR

状態はenumと明示的な状態機械で管理すること。

# 8. 今回の作業範囲

今回はPlant Doctor全体を完成させない。

最初の目標は、DT-EBML63Q2557上で動作するPlant Doctor Bring-upファームウェアを作ることである。

今回実装するもの：

1. 電源自己保持
2. ボード初期化
3. 1msまたは10ms周期タイマ
4. 1秒周期のLED点滅
5. LCD初期化
6. LCDへの固定文字表示
7. 基板上スイッチ入力
8. エラー状態表示
9. センサ、ポンプ、AI、ストレージ用インターフェース
10. LEXIDE-Ωでのビルド
11. MCU-Linkでの書き込み・デバッグ

起動後のLCD表示：

1行目：
PLANT DOCTOR

2行目：
BOARD TEST

スイッチを押した場合は、押されたスイッチ番号をLCDまたはLEDで確認できるようにする。

# 9. リポジトリ調査

最初に以下を調査すること。

- CommonFiles以下の全ファイル
- CommonFiles/Driver
- CommonFiles/Power
- CommonFiles/SoftSpi
- CommonFiles/Timer
- 各SampleProject
- .project
- .cproject
- .settings
- リンカスクリプト
- スタートアップコード
- インクルードパス
- 各サンプルが利用するCommonFiles
- LCD、電源制御、GPIO、タイマの依存関係

対象サンプル：

- AnalogSensor
- Fram
- GPIO
- Lcd
- MemsAccelerometer
- PowerControl
- PowerMonitoringAnalogInput
- Rtc
- Uart

# 10. Git作業

変更前に必ずGitで復元可能な状態を作る。

- Git未初期化ならgit init
- .gitignoreを確認
- vendor-original等の初期コミットを作る
- Gitユーザー情報不足などでコミットできない場合は、勝手にグローバル設定を変更せず報告する
- 削除や大規模変更前にはgit statusを確認する

# 11. ベースプロジェクトの選定

各サンプルを比較し、Plant Doctorのベースに最適なものを選定する。

優先候補：
- Lcd
- PowerControl
- GPIO

ただし、複数サンプルを無理に結合するより、新規プロジェクトを作ってCommonFilesを参照する方が安全なら、新規作成を選択すること。

判断基準：
- スタートアップとリンカ設定が正常
- ML63Q2557向け設定が正しい
- LCDを利用できる
- 電源自己保持に対応できる
- 相対パス化しやすい
- 将来のセンサとAI追加に適している

選択理由を文書化すること。

# 12. 作成する構成

次の構造を目標とする。

PlantDoctorWorkspace/
└─ PlantDoctor/
   ├─ app/
   │  ├─ App.c
   │  ├─ App.h
   │  ├─ AppStateMachine.c
   │  └─ AppStateMachine.h
   │
   ├─ board/
   │  ├─ Board.c
   │  ├─ Board.h
   │  ├─ PowerControlAdapter.c
   │  ├─ LedControl.c
   │  └─ SwitchControl.c
   │
   ├─ sensors/
   │  ├─ SensorManager.c
   │  ├─ SensorManager.h
   │  ├─ SoilMoistureSensor.c
   │  ├─ LeafTemperatureSensor.c
   │  └─ EnvironmentSensor.c
   │
   ├─ actuator/
   │  ├─ PumpControl.c
   │  └─ PumpControl.h
   │
   ├─ ai/
   │  ├─ PlantAi.c
   │  ├─ PlantAi.h
   │  ├─ PlantFeature.c
   │  └─ PlantFeature.h
   │
   ├─ ui/
   │  ├─ LcdUi.c
   │  └─ LcdUi.h
   │
   ├─ storage/
   │  ├─ PlantLog.c
   │  └─ PlantLog.h
   │
   ├─ config/
   │  └─ PlantDoctorConfig.h
   │
   ├─ src/
   │  └─ main.c
   │
   ├─ inc/
   ├─ .project
   ├─ .cproject
   └─ .settings/

ディレクトリ構成はLEXIDE-Ωの制約に合わせて変更してよいが、アプリ、ボード依存、センサ、AI、UI、アクチュエータを分離すること。

# 13. ベンダーコードの扱い

- CommonFilesはベンダー提供コードとして原則直接変更しない
- PlantDoctor側にAdapterまたはWrapperを作る
- CommonFilesを変更する必要がある場合は、理由を明記し、最小限にする
- 著作権表記を削除しない
- 配布条件が不明なコードを外部公開前提にしない

# 14. 実装規約

- C99またはプロジェクト既定のC規格
- 動的メモリ確保を使用しない
- 固定長バッファを使う
- 割り込み内で重い処理を行わない
- 割り込みではフラグまたはカウンタだけを更新する
- LCD、I2C、SPI、AI処理はmainループで実行する
- ハードウェアレジスタの直接操作をアプリ層へ書かない
- グローバル変数を必要以上に公開しない
- タイムアウトを設ける
- 無限待ちを作らない
- エラーコードを明示する
- 特定ユーザーの絶対パスを使用しない
- インクルードパスは相対パスまたはワークスペース変数を使う

# 15. サンプル削除方針

PlantDoctorがビルド可能になるまで既存サンプルを削除しない。

PlantDoctorのビルド確認後に、不要なSampleProjectを整理する。

- CommonFilesは削除しない
- PlantDoctorが参照するファイルは削除しない
- 判断不能なファイルは削除しない
- 削除前にGitで保存する
- 削除対象と理由を一覧化する

サンプルを完全削除するより、vendor_samplesまたはreferenceへ移動する方が安全なら、その方法を優先する。

# 16. 今回実装しないもの

次は今回のBring-upでは実装しない。

- Solist-AIの本学習
- 異常検知モデル
- FFT
- 自動給水判断
- ポンプの実駆動
- 植物センサの実ドライバ
- FTDI通信
- COM通信
- Solist-AI Scope
- PC Hostアプリ
- クラウド通信

ただし、後から追加できるヘッダー、状態、インターフェース、スタブは用意する。

# 17. 成果物

最後に以下を報告する。

- 調査したファイル
- 各サンプルの役割
- ベースに選んだサンプル
- 選定理由
- 作成したディレクトリ構造
- 作成・変更・削除したファイル
- CommonFilesとの依存関係
- ビルド結果
- LEXIDE-Ωへのインポート手順
- MCU-Linkでの書き込み手順
- 実機確認手順
- LCDが表示されない場合の切り分け
- 未実装項目
- 次の開発ステップ

まず変更を行わずにリポジトリ全体を調査し、調査結果と実施計画を提示してください。

その後、Gitで初期状態を保存してから実装してください。

確認なしにベンダーコード、CommonFiles、リンカスクリプト、スタートアップコードを削除または大幅変更しないでください。