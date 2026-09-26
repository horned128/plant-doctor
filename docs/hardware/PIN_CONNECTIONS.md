# エッジAIテラリウム 現在のピン接続状況

## 1. 前提

本ドキュメントは、現在のプロトタイプ配線を整理したものである。

- 使用ボード：Solist-AI™搭載評価ボード `DT-EBML63Q2557`
- 付属MEMS加速度センサ：**未使用**
- 14ピンIDCハーネス：**未使用**
- I2C HUB V1.0：**使用**
- CN3とI2C HUB入力をジャンパ線で接続する
- I2Cセンサ3台はGravityケーブルでHUBへ接続し、SCL / SDA / VCC / GNDを並列接続する
- ポンプは評価ボードから直接給電せず、外部12V電源から駆動する

---

# 2. 全体接続概要

```text
                         DT-EBML63Q2557
                         Sol    ist-AI™ Board
                               │
          ┌────────────────────┼─────────────────────┐
          │                    │                     │
          │ CN3                │ CN6                 │ CN5
          │ I2C / 3.3V         │ Analog / 5V        │ I/O / SSR
          │                    │                     │
          ▼                    ▼                     ▼

  SEN0206 葉温        SEN0193 土壌水分       SEN0204 液面
  BME280 温湿度・気圧 ・5V                  ・5V
  SEN0228 照度        ・Analog Input         ・絶縁入力 IN0
  ※3台をI2C並列

                                                   CN5 OUT0
                                                       │
                                                       ▼
                                              DFR0457 MOSFET
                                                       │
                                             外部12V電源
                                                       │
                                                       ▼
                                                    給水ポンプ
```

---

# 3. CN3：I2Cセンサ接続

## 使用するCN3ピン

| CN3 Pin | 信号 | 用途 |
|---:|---|---|
| 1 | SCL / P73 | I2Cクロック |
| 2 | GND | I2Cセンサ共通GND |
| 3 | SDA / P74 | I2Cデータ |
| 5 | Power Out | I2Cセンサ電源 3.3V |
| 8 | Power Out | DFR0457制御側3.3Vに使用 |
| 13 | GND | DFR0457制御側GNDに使用 |

I2Cセンサはすべて **3.3V駆動** とする。

## ボード設定

- `JP1`：**3.3V側**
- CN3のPower Outを3.3Vとして使用する

## 3.1 SEN0206 赤外線葉温センサ

| SEN0206 | 接続先 |
|---|---|
| VCC | CN3-5（3.3V） |
| GND | CN3-2（GND） |
| SCL | CN3-1（SCL） |
| SDA | CN3-3（SDA） |

```text
SEN0206
 VCC ───────── CN3-5
 GND ───────── CN3-2
 SCL ───────── CN3-1
 SDA ───────── CN3-3
```

用途：

- 葉温
- 葉温の変化速度
- 葉温と気温との差
- 水やり前後の葉温変化

## 3.2 BME280 温湿度・気圧センサ

BME280モジュールは3.3V駆動とし、CN3またはI2C HUB経由で接続する。
I2Cアドレスは `0x76`（SDO=GND）または `0x77`（SDO=VCC）に対応し、ファームウェア側で自動検出される。

| BME280ピン | 信号 | 接続先 |
|---|---|---|
| VCC / VIN | 電源 | CN3-5（3.3V） |
| GND | GND | CN3-2（GND） |
| SCL | I2C SCL | CN3-1（SCL） |
| SDA | I2C SDA | CN3-3（SDA） |

```text
BME280
 VCC ───────── CN3-5 (3.3V)
 GND ───────── CN3-2 (GND)
 SCL ───────── CN3-1 (SCL)
 SDA ───────── CN3-3 (SDA)
```

用途：

- 気温
- 湿度
- 気圧
- 葉温－気温差の算出
- 植物周辺環境の評価

## 3.3 SEN0228 照度センサ

| SEN0228 | 接続先 |
|---|---|
| VCC | CN3-5（3.3V） |
| GND | CN3-2（GND） |
| SCL | CN3-1（SCL） |
| SDA | CN3-3（SDA） |

```text
SEN0228
 VCC ───────── CN3-5
 GND ───────── CN3-2
 SCL ───────── CN3-1
 SDA ───────── CN3-3
```

用途：

- 照度
- 日照不足の推定
- 葉温上昇と照度の関係評価

## 3.4 I2C全体

CN3をI2C HUBの入力へ接続し、3台のI2CセンサをHUBの各ポートへ接続する。
HUBは受動分岐のため、電気的には同じ4本への並列接続となる。

```text
CN3-5  3.3V ── HUB VCC ─┬─ SEN0206 VCC
                         ├─ BME280 VCC
                         └─ SEN0228 VCC

CN3-2  GND  ─── HUB GND ─┬─ SEN0206 GND
                          ├─ BME280 GND
                          └─ SEN0228 GND

CN3-1  SCL  ─── HUB SCL ─┬─ SEN0206 SCL
                          ├─ BME280 SCL
                          └─ SEN0228 SCL

CN3-3  SDA  ─── HUB SDA ─┬─ SEN0206 SDA
                          ├─ BME280 SDA
                          └─ SEN0228 SDA
```

### 注意

- 3台は同一I2Cバス上に接続する
- I2Cアドレスが重複しないことをソフト起動時に確認する
- 配線はできるだけ短くする
- SCL / SDAを長く引き回す場合は通信速度を下げることも検討する
- 電源投入前にVCCとGNDの短絡がないことをテスターで確認する

---

# 4. CN6：SEN0193 土壌水分センサ

CN6はアナログ入力として使用する。

## CN6ピン

| CN6 Pin | 信号 | 接続 |
|---:|---|---|
| 1 | Power Out | SEN0193 VCC |
| 2 | Analog Input | SEN0193 Signal |
| 3 | GND | SEN0193 GND |

## 配線

```text
SEN0193
 VCC    ───────── CN6-1  5V
 Signal ───────── CN6-2  Analog Input
 GND    ───────── CN6-3  GND
```

## ボード設定

現在の想定設定：

- `JP4`：SHORT
- `JP7`：SHORT（DCカップリング）
- `JP6`：5V側
- `JP5`：OPEN
- アナログ入力ゲイン：1倍

用途：

- 土壌水分値
- 土壌水分の低下速度
- 水やり後の水分上昇量
- 水やり成功 / 失敗判定

---

# 5. SEN0204 液面センサ

SEN0204は水タンクの液面検出に使用する。

## 電源

SEN0204は5Vで使用する。

CN6-1 / CN6-3をSEN0193と共用して分岐する。

```text
CN6-1 5V ─────┬─ SEN0193 VCC
               └─ SEN0204 VCC

CN6-3 GND ────┬─ SEN0193 GND
               └─ SEN0204 GND
```

## SEN0204 → CN5

| SEN0204線 | 信号 | 接続先 |
|---|---|---|
| 茶 | VCC | CN6-1（5V） |
| 青 | GND | CN6-3（GND）およびCN5-6 |
| 黄 | OUT | CN5-5（IN0+） |
| 黒 | ADJ | 未接続 |

```text
SEN0204

茶 VCC ───────── CN6-1 5V

青 GND ─────┬─── CN6-3 GND
             └─── CN5-6 IN0-

黄 OUT ───────── CN5-5 IN0+

黒 ADJ ───────── NC
```

用途：

- 水タンク残量検知
- タンク空検知
- 水やり失敗時の原因切り分け

---

# 6. CN5：DFR0457 ポンプ制御

DT-EBML63Q2557のSSR出力 `OUT0` を、DFR0457の制御信号ON/OFFに使用する。

## CN5使用ピン

| CN5 Pin | 信号 | 接続 |
|---:|---|---|
| 9 | OUT0a | CN3-8の3.3Vを入力 |
| 10 | OUT0c | DFR0457 Signalへ |

OUT0は無電圧接点として使用する。

## DFR0457制御側

| DFR0457 P1 | 信号 | 接続先 |
|---:|---|---|
| 1 | Signal | CN5-10 OUT0c |
| 2 | VCC | CN3-8 3.3V |
| 3 | GND | CN3-13 GND |

## 配線

```text
CN3-8 3.3V ─────┬──── DFR0457 P1-2 VCC
                 │
                 └──── CN5-9 OUT0a

CN5-10 OUT0c ───────── DFR0457 P1-1 Signal

CN3-13 GND ─────────── DFR0457 P1-3 GND
```

### 動作

```text
Solist-AI™ OUT0 OFF
    ↓
CN5-9 - CN5-10 開放
    ↓
DFR0457 Signal = OFF
    ↓
ポンプ停止


Solist-AI™ OUT0 ON
    ↓
CN5-9 - CN5-10 導通
    ↓
3.3V → DFR0457 Signal
    ↓
ポンプ動作
```

---

# 7. DFR0457：12Vポンプ駆動

ポンプ電源はSolist-AI™評価ボードとは別系統の12V電源を使用する。

## パワー側配線

```text
12V電源 +
    │
    ▼
DFR0457 VIN

DFR0457 VOUT
    │
    ▼
ポンプ +

ポンプ -
    │
    ▼
DFR0457 GND
    ▲
    │
12V電源 -
```

| DFR0457端子 | 接続 |
|---|---|
| VIN | 外部12V電源 + |
| GND | 外部12V電源 - / ポンプ - |
| VOUT | ポンプ + |

### 注意

- ポンプをSolist-AI™評価ボードの3.3V / 5Vから直接駆動しない
- ポンプは必ず外部12V電源から駆動する
- モーターには逆起電力対策を行う
- 必要に応じてヒューズを12V電源ラインに追加する

---

# 8. 現在未使用の端子

## CN3

以下は現在未使用。

- CN3-4 GND
- CN3-6 INT2
- CN3-7 INT1
- CN3-9 MOSI
- CN3-10 MISO
- CN3-11 GND
- CN3-12 SCK
- CN3-14 CS

付属MEMSセンサを使用しないため、SPI関連端子は未使用とする。

## CN5

現在使用：

- IN0：液面センサ
- OUT0：ポンプ制御

その他の入力 / 出力は未使用。

---

# 9. ピン接続一覧

| デバイス | 信号 | DT-EBML63Q2557 |
|---|---|---|
| SEN0206 | VCC | CN3-5 |
| SEN0206 | GND | CN3-2 |
| SEN0206 | SCL | CN3-1 |
| SEN0206 | SDA | CN3-3 |
| BME280 | VCC | CN3-5 |
| BME280 | GND | CN3-2 |
| BME280 | SCL | CN3-1 |
| BME280 | SDA | CN3-3 |
| SEN0228 | VCC | CN3-5 |
| SEN0228 | GND | CN3-2 |
| SEN0228 | SCL | CN3-1 |
| SEN0228 | SDA | CN3-3 |
| SEN0193 | VCC | CN6-1 |
| SEN0193 | Signal | CN6-2 |
| SEN0193 | GND | CN6-3 |
| SEN0204 | VCC | CN6-1 |
| SEN0204 | GND | CN6-3 / CN5-6 |
| SEN0204 | OUT | CN5-5 |
| DFR0457 | VCC | CN3-8 |
| DFR0457 | GND | CN3-13 |
| DFR0457 | Signal | CN5-10 |
| CN5 OUT0a | 3.3V入力 | CN3-8 → CN5-9 |

---

# 10. 現在の配線状態を一枚で見る

```text
                            DT-EBML63Q2557
                     ┌─────────────────────────┐
                     │                         │
     CN3-5 3.3V ─────┼── I2C HUB ┬─ SEN0206 VCC
                     │           ├─ BME280 VCC
                     │           └─ SEN0228 VCC
                     │                         │
     CN3-2 GND ──────┼── I2C HUB ┬─ SEN0206 GND
                     │           ├─ BME280 GND
                     │           └─ SEN0228 GND
                     │                         │
     CN3-1 SCL ──────┼── I2C HUB ┬─ SEN0206 SCL
                     │           ├─ BME280 SCL
                     │           └─ SEN0228 SCL
                     │                         │
     CN3-3 SDA ──────┼── I2C HUB ┬─ SEN0206 SDA
                     │           ├─ BME280 SDA
                     │           └─ SEN0228 SDA
                     │                         │
     CN6-1 5V ───────┼──┬─ SEN0193 VCC       │
                     │  └─ SEN0204 VCC       │
                     │                         │
     CN6-2 Analog ◀──┼──── SEN0193 Signal    │
                     │                         │
     CN6-3 GND ──────┼──┬─ SEN0193 GND       │
                     │  └─ SEN0204 GND       │
                     │                         │
     CN5-5 IN0+ ◀────┼──── SEN0204 OUT       │
     CN5-6 IN0- ─────┼──── SEN0204 GND       │
                     │                         │
     CN3-8 3.3V ─────┼──┬─ DFR0457 VCC       │
                     │  └─ CN5-9 OUT0a       │
     CN5-10 OUT0c ───┼──── DFR0457 Signal    │
     CN3-13 GND ─────┼──── DFR0457 GND       │
                     └─────────────────────────┘

                                  DFR0457
                             ┌─────────────┐
  External 12V + ───────────▶│ VIN         │
                             │             │
                             │ VOUT ───────┼──▶ Pump +
  External 12V - ───────────▶│ GND ───────┼──▶ Pump -
                             └─────────────┘
```

---

# 11. 電源投入前チェック

- [ ] CN3のPower Outが3.3Vに設定されている
- [ ] SEN0206 / BME280 / SEN0228が3.3V系に接続されている
- [ ] I2CのSCLとSDAを逆接続していない
- [ ] SEN0193 SignalがCN6-2へ接続されている
- [ ] CN6アナログ入力が0～3.3V範囲で使用される設定になっている
- [ ] SEN0204を5Vで駆動している
- [ ] SEN0204 OUTがCN5 IN0+へ接続されている
- [ ] DFR0457制御側を3.3Vで駆動している
- [ ] ポンプ用12V電源とボード電源を混同していない
- [ ] 12VをSolist-AI™のCN3 / CN6へ入力していない
- [ ] テスターでVCC-GND間の短絡がないことを確認した
- [ ] まずポンプを外した状態でセンサ値とSSR出力を確認する

---

## 更新ルール

配線変更時は、本ファイルの「ピン接続一覧」と「現在の配線状態を一枚で見る」を同時に更新する。
