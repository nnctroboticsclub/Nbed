# Nbed

Nbed は、**STM32シリーズ** で GPIO、ADC、PWM、I²C、UART、CAN、
エンコーダを扱うための C++ ライブラリです。STM32 HAL の初期化手順を隠しつつ、使用する
タイマ・通信機能・ピンをコードに明示して、間違った組み合わせは `Initialize()` の失敗として
検出します。

初心者はまず `#include "nbed.h"` と `System::Initialize()` だけ覚えれば大丈夫です。  
こちらの [サンプル](最初に動かす) にチュートリアルとしてLチカの例があります。

## 目次

- [Nbed](#nbed)
  - [目次](#目次)
  - [最初に動かす](#最初に動かす)
    - [必要なもの](#必要なもの)
  - [共通の考え方](#共通の考え方)
    - [ピン名の書き方](#ピン名の書き方)
    - [初期化の順番](#初期化の順番)
    - [`printf` の出力先](#printf-の出力先)
  - [GPIO（LED・ボタン）](#gpioledボタン)
    - [出力: LED を点灯する](#出力-led-を点灯する)
    - [入力: スイッチを読む](#入力-スイッチを読む)
  - [アナログ入力（ADC）](#アナログ入力adc)
  - [PWM（サーボ・モータドライバ）](#pwmサーボモータドライバ)
    - [PWM で特に重要な制約](#pwm-で特に重要な制約)
  - [I2C（センサ）](#i2cセンサ)
  - [UART（PC・外部機器とのシリアル通信）](#uartpc外部機器とのシリアル通信)
    - [既定の `printf`](#既定の-printf)
    - [任意の UART を使う](#任意の-uart-を使う)
  - [CAN](#can)
  - [エンコーダ](#エンコーダ)
  - [時間待ち・計時・単位](#時間待ち計時単位)
    - [待機](#待機)
    - [経過時間](#経過時間)
    - [単位付きの値](#単位付きの値)
  - [対応ピン一覧](#対応ピン一覧)
    - [基板上でまず使える信号](#基板上でまず使える信号)
    - [GPIO](#gpio)
    - [ADC1](#adc1)
    - [PWM](#pwm)
    - [I²C](#ic)
    - [UART](#uart)
    - [CAN](#can-1)
    - [エンコーダ](#エンコーダ-1)
  - [上級者向け: マクロとビルドの仕組み](#上級者向け-マクロとビルドの仕組み)
    - [マクロ](#マクロ)
    - [PlatformIO に残す設定](#platformio-に残す設定)
    - [クロックを変更する](#クロックを変更する)
  - [制約と未対応機能](#制約と未対応機能)
  - [コントリビューター](#コントリビューター)
  - [ライセンス](#ライセンス)
  - [バージョン履歴](#バージョン履歴)

## 最初に動かす

### 必要なもの

- NUCLEO-F446RE
- ST-LINK 用 USB ケーブル
- PlatformIO が使える環境（VS Code + PlatformIO IDE など）

最初はNucleo基板上の LED（`PA5`）を点滅させるだけの、次のプログラムがおすすめです。

```cpp
#include "nbed.h"

int main() {
  System::Initialize(); // クロック等の初期化

  DigitalOut led(kPA5); // LEDを定義

  led.Initialize();     // LEDを初期化

  while (true) {
    led = true;       // 点灯 (led.Write(true) とも書ける)
    SleepFor(500ms);  // 500ms 待つ
  
    led = false;      // 消灯 (led.Write(false) とも書ける)
    SleepFor(500ms);  // 500ms 待つ
  }
}
```

`System::Initialize()` は、HAL、クロック、時間計測、DMA を初期化します。`main()` の先頭で
**一度だけ**呼び出してください。`SleepFor()` の `ms` と `s` は、それぞれミリ秒・秒を表します。

`Initialize()` は、初期化に失敗した場合に `false` を返します。  
`false` なら、選んだピンや周辺機能の組み合わせが使えない、同じ資源を
すでに使っている、または設定値が不正です。最初のうちは戻り値を必ず確認しましょう。

```cpp
DigitalOut output(kPA5);
if (!output.Initialize()) {
  System::ErrorHandler();  // 割り込みを止めて停止する既定のエラー処理
}
```

## 共通の考え方

### ピン名の書き方

`kPA5` は「GPIOA の 5 番ピン」、`kPB8` は「GPIOB の 8 番ピン」です。STM32 のポート名に加え、
NUCLEO-F446RE の Arduino 互換コネクタ名も `kD1` や `kA3` のように指定できます。どちらも同じ
ピンを表す別名なので、すべての機能で混在なく使用できます。基板上の信号名と対応表は
[対応ピン一覧](#対応ピン一覧)を確認してください。

### 初期化の順番

1. `System::Initialize()` を呼ぶ。
2. 使用したい機能のオブジェクトを作る。
3. `Initialize()` が成功したことを確認する。
4. 読み書きや通信を始める。

同じオブジェクトに対して `Initialize()` を複数回呼んでも問題ありません。いっぽうで、同じ
UART・I²C・CAN 周辺機能を二つのオブジェクトで同時に初期化することはできません。

### `printf` の出力先

既定では `System::Initialize()` が USART2（RX: `PA3`、TX: `PA2`、921600 bps）をコンソールに
設定します。そのため `printf("value = %d\\n", value);` は ST-LINK の仮想 COM ポートへ出力
できます。詳しい切り替え方は [UART](#uartpc外部機器とのシリアル通信) と
[マクロ](#マクロ)を参照してください。

## GPIO（LED・ボタン）

### 出力: LED を点灯する

NUCLEO-F446RE のユーザー LED LD2 は `PA5` です。`true` は High、`false` は Low を出力します。
この基板では High で LED が点灯します。

```cpp
DigitalOut led(kPA5);

led.Initialize();

led.Write(true);   // 点灯
led.Write(false);  // 消灯
led = true;        // Write(true) と同じ
```

コンストラクタの 2 番目の引数は、初期化した直後の出力値です。たとえば
`DigitalOut motor_enable(kPB0, false);` とすれば、初期化時に Low を出力します。

### 入力: スイッチを読む

`DigitalIn` の `Read()` はピンの現在の電圧を `bool` で返します。外付けスイッチは、押した時に
High になる接続か Low になる接続かを回路図で確認してください。基板上の B1 USER は `PC13` に
接続されています。

```cpp
DigitalIn button(kPC13, GpioPull::kNone);

button.Initialize();

if (button.Read()) {
  // High のときに実行する処理
}
```

未接続の入力は High/Low が安定しません。外部抵抗がないスイッチには `GpioPull::kUp` または
`GpioPull::kDown` を選び、押していない時の電圧を決めます。

```cpp
DigitalIn switch_input(kPB1, GpioPull::kUp);
// スイッチで PB1 を GND へつなぐ回路なら、押した時は false（Low）
```

`DigitalOut` には `DigitalIn` をそのまま代入できます。

```cpp
led = button;  // button の現在の値を LED へ出力
```

## アナログ入力（ADC）

`AnalogIn` は ADC1 の 12 bit 変換を使います。`ReadRatio()` は `0.0` から `1.0` の比率、
`ReadVoltage()` は指定した基準電圧から求めた電圧を返します。

```cpp
AnalogIn battery(kPA0);  // 基準電圧は既定で 3.3 V

battery.Initialize();

const float ratio = battery.ReadRatio();      // 例: 0.50
const float voltage = battery.ReadVoltage();  // 例: 約 1.65 V
```

基準電圧が 3.3 V 以外の基板では、コンストラクタの 2 番目の引数で指定します。

```cpp
AnalogIn sensor(kPA1, 3.0F);
```

ADC 端子に入力できるのは **0 V から基準電圧まで**です。測りたい電圧が 3.3 V を超える場合は、
必ず抵抗分圧回路を用意してください。ADC は読み出すたびに一回変換するため、高速連続サンプリング
や ADC の DMA はこのライブラリの対象外です。

## PWM（サーボ・モータドライバ）

`Pwm` はタイマの一つのチャンネルを PWM 出力にします。サーボなら 50 Hz で初期化し、パルス幅を
指定する方法が分かりやすいです。

```cpp
Pwm servo(kTim3, kCh1, kPB4, 50);  // TIM3 CH1 / PB4 / 50 Hz

servo.Initialize();

servo.SetPulseWidthUs(1'000);  // 1.0 ms
SleepFor(1s);
servo.SetPulseWidthUs(1'500);  // 1.5 ms
SleepFor(1s);
servo.SetPulseWidthUs(2'000);  // 2.0 ms
```

デューティ比で指定することもできます。`0.0F` から `1.0F` の範囲で、範囲外の値は 0 または 1 に
丸められます。

```cpp
pwm.SetDutyCycle(0.25F);  // 25 %
pwm.SetFrequencyHz(20'000);
```

### PWM で特に重要な制約

- コンストラクタの「タイマ・チャンネル・ピン」は、[対応ピン一覧](#対応ピン一覧)の組み合わせを
  そのまま使います。たとえば `kTim3, kCh1, kPB4` は使えますが、`kTim3, kCh1, kPA5` は使えません。
- 同じタイマの複数チャンネルは使えますが、**周波数を共有**します。最初のチャンネルと異なる
  周波数で初期化すると `false` になります。
- 同じタイマで PWM とエンコーダを併用することはできません。競合した `Initialize()` は `false` を
  返します。
- `SetFrequencyHz()` は、そのタイマで PWM を一つだけ使っている時に限り変更できます。
- `Stop()` を呼ぶか、`Pwm` オブジェクトが破棄されると出力を停止してタイマを解放します。

サーボやモータは NUCLEO の 3.3 V ピンから給電しないでください。外部電源を使い、GND だけは
NUCLEO と接続します。

## I2C（センサ）

`I2c` は I²C マスターです。SCL、SDA、通信速度を指定してから、送受信用の `I2CMessage` を渡します。
アドレスは **7 bit の値をそのまま**書きます。`0x68 << 1` のように左シフトする必要はありません。

```cpp
#include <array>

I2c i2c(kI2c1, kPB6, kPB7, 400'000);

i2c.Initialize();

std::array<uint8_t, 2> command{0x75, 0x00};
I2CMessage write_message{
    .address = 0x68,
    .size = static_cast<uint8_t>(command.size()),
    .data = command.data(),
};
if (!i2c.Send(write_message)) {
  System::ErrorHandler();
}

std::array<uint8_t, 1> received{};
I2CMessage read_message{
    .address = 0x68,
    .size = static_cast<uint8_t>(received.size()),
    .data = received.data(),
};
if (!i2c.Receive(read_message)) {
  System::ErrorHandler();
}
```

対応速度は 400 kHz 以下です。長い配線や複数のデバイスをつなぐ場合は、モジュール側の
抵抗だけに頼らず、適切なプルアップ抵抗を SCL/SDA に入れてください。`Send()` と `Receive()` は
完了まで待つ（ブロッキングする）関数です。

## UART（PC・外部機器とのシリアル通信）

### 既定の `printf`

何も設定しなければ、`System::Initialize()` が USART2（RX: `PA3`、TX: `PA2`、921600 bps）を
初期化します。ST-LINK の仮想 COM ポートを開けば、次のような出力を確認できます。

```cpp
#include <cstdio>

printf("Hello, Nbed!\\n");
```

この既定設定が USART2 を使用中なので、同じ USART2 を自分で初期化する場合は
`NBED_NO_DEFAULT_SERIAL` を定義してください。定義方法は[マクロ](#マクロ)を参照します。

### 任意の UART を使う

`Send()` は `std::span` を受け取るため、`std::array` や `std::vector` など連続したバイト列を
直接渡せます。

```cpp
#include <array>

Uart device(kUsart3, kPC11, kPB10, 115'200);

device.Initialize();

const std::array<uint8_t, 3> message{'O', 'K', '\\n'};
device.Send(message);
```

自分で初期化した UART を `printf` の出力先にするには `SetConsole()` を呼びます。

```cpp
Uart pc(kUsart2, kPA3, kPA2, 921'600);
if (pc.Initialize()) {
  pc.SetConsole();
}
```

受信割り込みを使う時は、コールバックを設定してから `EnableRxInterrupt()` を呼びます。
コールバックには **受信した 1 バイト**が渡され、割り込み処理の中で実行されます。コールバック内で
`SleepFor()`、`printf()`、長い送信、動的メモリ確保などの時間のかかる処理は行わず、値を保存する
だけにしてください。

```cpp
volatile uint8_t latest_byte = 0;

device.SetRxCallback([](UartRxMessage message) {
  latest_byte = message[0];
});
if (!device.EnableRxInterrupt()) {
  System::ErrorHandler();
}
```

`Receive()` は指定したバイト数を受信するまで待つブロッキング関数です。

## CAN

Nbed の CAN は Classic CAN（最大 8 byte/frame）です。NUCLEO のピンだけでは CAN バスに
つなげません。**別途 CAN トランシーバ**（例: 3.3 V 対応品）が必要です。CANH/CANL の配線、
両端の終端抵抗、共通 GND を正しく接続してください。CAN FD には対応していません。

```cpp
Can can(kCan1, kPB8, kPB9);  // CANを定義

can.Initialize();  // CANを初期化

can.Start();  // CANの送受信を開始

// 送信するメッセージを作成 (ID: 0x123, データ長: 3, データ: {0x4E, 0x42, 0x45})
CanMessage message{
    .id = 0x123,
    .size = 3,
    .data = {0x4E, 0x42, 0x45},
};

can.Send(message);  // 送信
```

既定では標準 ID・拡張 ID とも受信します。標準 ID `0x123` だけを通すには、受信開始前または後に
フィルタを設定できます。

```cpp
can.SetRxFilter(0x123, 0x7ff);
```

受信を取りこぼしにくくするには、ポーリングの `Receive()` より受信割り込みを推奨します。
UART と同様、コールバックは割り込み文脈で短く終える必要があります。

```cpp
// 受信したか同課のフラグ (割り込みで書き換えられるので volatile)
volatile bool has_message = false;

// 受信割り込みのコールバックを設定
can.SetRxCallback([](const CanMessage&) {
  has_message = true;
});

can.EnableRxInterrupt();  // 受信割り込みを有効化
```

`CanConfig` を使うと、ループバックや割り込み優先度も指定できます。外部配線なしの動作確認には
`kLoopback` が便利です。

```cpp
Can test_can({
    .peripheral = kCan1,
    .rx = kPB8,
    .tx = kPB9,
    .bitrate = 1'000'000,
    .mode = kLoopback,
});
```

## エンコーダ

`Encoder` は A/B 相のインクリメンタルエンコーダをタイマのエンコーダモードで読みます。
4 番目の引数には、`GetCount()` が 1 回転で進む**実効カウント数**を指定してください。エンコーダの
仕様にある PPR と、逓倍後のカウント数は異なることがあります。

```cpp
Encoder encoder(EncoderTimer::kTim3, kPB4, kPB5, 2048);

encoder.Initialize();

const int count = encoder.GetCount();
const float angle_deg = encoder.GetAngle().GetAsDegree();

encoder.ResetCount();
```

タイマ 1/2/3 を使うため、同じタイマを PWM とエンコーダに同時使用することはできません。
`TIM1` と `TIM3` は 16 bit、`TIM2` は 32 bit のカウンタです。非常に長く回転させる場合は、
カウンタの周回を考慮して定期的に `GetCount()` を読み取ってください。

## 時間待ち・計時・単位

### 待機

`SleepFor()` は CPU を待機させます。制御周期の厳密さが必要な処理や、割り込みコールバックの中では
使わないでください。

```cpp
SleepFor(10ms);
SleepFor(1s);
```

### 経過時間

`Timer` はマイクロ秒単位の経過時間を測ります。

```cpp
Timer timer;
timer.Start();
// 計測したい処理
timer.Stop();

const uint64_t elapsed_us = timer.ElapsedMicroseconds();
```

### 単位付きの値

`Angle`、`Velocity`、`Current` には変換用の型とリテラルがあります。

```cpp
const Angle target = 90.0_deg;
const Velocity speed = 120.0_rpm;
const Current limit = 500.0_mA;

const float radians = target.GetAsRadian();
```

## 対応ピン一覧

以下は NUCLEO-F446RE / STM32F446RE で Nbed が受け付ける組み合わせです。表にない
組み合わせを指定すると、初期化は失敗します。

### 基板上でまず使える信号

| 用途 | STM32 ピン | NUCLEO の信号 |
| --- | --- | --- |
| LD2 ユーザー LED | `PA5` | Arduino D13 |
| B1 USER ボタン | `PC13` | B1 |
| 既定の UART RX | `PA3` | Arduino D0 |
| 既定の UART TX | `PA2` | Arduino D1 |
| I²C1 SCL | `PB8` | Arduino D15 |
| I²C1 SDA | `PB9` | Arduino D14 |
| ADC の例 | `PA0` | Arduino A0 |

Arduino 互換コネクタの表記を使うこともできます。例えばNucleoに搭載されている LED は
`kD13`（`kPA5` と同じ）、UART の送信は `kD1`（`kPA2` と同じ）、アナログ入力 A3 は
`kA3`（`kPB0` と同じ）です。

| Arduino 表記 | STM32 ピン | Arduino 表記 | STM32 ピン |
| --- | --- | --- | --- |
| `kD0` | `kPA3` | `kD1` | `kPA2` |
| `kD2` | `kPA10` | `kD3` | `kPB3` |
| `kD4` | `kPB5` | `kD5` | `kPB4` |
| `kD6` | `kPB10` | `kD7` | `kPA8` |
| `kD8` | `kPA9` | `kD9` | `kPC7` |
| `kD10` | `kPB6` | `kD11` | `kPA7` |
| `kD12` | `kPA6` | `kD13` | `kPA5` |
| `kD14` | `kPB9` | `kD15` | `kPB8` |
| `kA0` | `kPA0` | `kA1` | `kPA1` |
| `kA2` | `kPA4` | `kA3` | `kPB0` |
| `kA4` | `kPC1` | `kA5` | `kPC0` |

基板のコネクタ上の詳しい場所は、ST の
[NUCLEO-64 ユーザーマニュアル](https://www.st.com/resource/en/user_manual/dm00105823.pdf)も参照してください。

### GPIO

デジタル入出力には `PA0–PA15`、`PB0–PB10`、`PB12–PB15`、`PC0–PC15`、`PD2`、`PH0–PH1` を
使えます。`kNC` は未接続を表す予約値で、初期化には使えません。

### ADC1

| ADC1 の入力 | 対応ピン |
| --- | --- |
| CH0–CH7 | `PA0–PA7` |
| CH8–CH9 | `PB0–PB1` |
| CH10–CH15 | `PC0–PC5` |

### PWM

| タイマ | チャンネル | 対応ピン |
| --- | --- | --- |
| TIM1 | CH1 / CH2 / CH3 / CH4 | `PA8` / `PA9` / `PA10` / `PA11` |
| TIM2 | CH1 | `PA0`, `PA5`, `PA15`, `PB8` |
| TIM2 | CH2 | `PA1`, `PB3`, `PB9` |
| TIM2 | CH3 | `PA2`, `PB10` |
| TIM2 | CH4 | `PA3`, `PB2` |
| TIM3 | CH1 | `PA6`, `PB4`, `PC6` |
| TIM3 | CH2 | `PA7`, `PB5`, `PC7` |
| TIM3 | CH3 | `PB0`, `PC8` |
| TIM3 | CH4 | `PB1`, `PC9` |

### I²C

| 周辺機能 | SCL / SDA の組み合わせ |
| --- | --- |
| I2C1 | `PB6` / `PB7`、`PB8` / `PB9` |
| I2C2 | `PB10` / `PB3`、`PB10` / `PC12` |
| I2C3 | `PA8` / `PB4`、`PA8` / `PC9` |

### UART

| 周辺機能 | RX | TX |
| --- | --- | --- |
| USART2 | `PA3` | `PA2` |
| USART3 | `PC5` または `PC11` | `PB10` または `PC10` |
| UART4 | `PA1` または `PC11` | `PA0` または `PC10` |

### CAN

| 周辺機能 | RX / TX の組み合わせ |
| --- | --- |
| CAN1 | `PA11` / `PA12`、`PB8` / `PB9` |
| CAN2 | `PB5` / `PB6`、`PB12` / `PB13` |

### エンコーダ

| タイマ | A 相（CH1） | B 相（CH2） |
| --- | --- | --- |
| TIM1 | `PA8` | `PA9` |
| TIM2 | `PA0`, `PA5`, `PA15`, `PB8` | `PA1`, `PB3`, `PB9` |
| TIM3 | `PA6`, `PB4`, `PC6` | `PA7`, `PB5`, `PC7` |

## 上級者向け: マクロとビルドの仕組み

### マクロ

マクロは `platformio.ini` の `build_flags` で定義します。ライブラリ本体にも影響させたい
`NBED_NO_DEFAULT_SERIAL` は、ソースコード内の `#define` ではなく**ビルドフラグで定義**して
ください。

```ini
[env:my_board]
build_flags =
  -DNBED_NO_DEFAULT_SERIAL
```

| マクロ | 変更される仕様 | 使う場面 |
| --- | --- | --- |
| `NBED_NO_DEFAULT_SERIAL` | `System::Initialize()` が USART2 / `PA3` / `PA2` の既定コンソールを作らなくなる。`printf` の出力先も自動では設定されない。 | USART2 を通信相手に使う、別の UART をコンソールにする。 |
| `NBED_NO_GLOBAL_NAMES` | `kPA5`、`kTim3`、`DigitalOut`、`CanMessage` などの短い名前をグローバル名前空間へ出さない。 | 大きな C++ プロジェクトで名前の衝突を防ぐ。 |
| `NBED_NO_GLOBAL_FUNCTIONS` | グローバルの `SleepFor()` を出さない。 | 同名の関数との衝突を防ぐ。 |

`NBED_NO_GLOBAL_NAMES` を使う場合は、型と列挙値を完全修飾します。

```cpp
#define NBED_NO_GLOBAL_NAMES
#include "nbed.h"

nbed::f4::DigitalOut led(nbed::interface::PinName::kPA5);
```

`NBED_NO_GLOBAL_FUNCTIONS` を使った場合の待機は、次のように書けます。

```cpp
nbed::f4::ClockManager::GetClock().Sleep(std::chrono::milliseconds{100});
```

### PlatformIO に残す設定

Nbed の `library.json` は `extra_script.py` を呼び、ライブラリの配置場所に応じて次を自動設定します。

- Nbed の `include` ディレクトリを、アプリ・STM32 HAL・ライブラリ自身の検索パスへ追加する。
- `-O3`、LTO、C++23、警告、セクション削除、浮動小数点 `printf` を、必要なビルド環境すべてへ
  同じように適用する。
- PlatformIO 標準の `-Os` と `-std=gnu++14` を外す。
- 同梱する `STM32F446RETX_FLASH.ld` を最終リンクへ指定し、変更時に再リンクする。

つまり、これらのフラグやリンカスクリプトを利用側の `platformio.ini` にコピーする必要はありません。
ただし PlatformIO がボードと STM32Cube フレームワークを選ぶ処理は、ライブラリを読み込むより前に
行われます。そのため、利用側には少なくとも次の設定が必要です。

```ini
[env:nucleo_f446re]
platform = ststm32@20.0.0
framework = stm32cube
board = nucleo_f446re
board_build.stm32cube.custom_config_header = yes
```

`custom_config_header = yes` は、Nbed に同梱した `stm32f4xx_hal_conf.h` を STM32Cube HAL が使う
ための指定です。この一行はライブラリのスクリプトだけでは代替できません。HAL 自身のビルド開始前に
PlatformIO が読む設定だからです。

上記のバージョンはこの開発リポジトリで検証した組み合わせです。別のバージョンを使う場合も、
ビルドが通ることと実機での動作を確認してください。最適化・LTO・リンカスクリプトの設定を独自に
上書きする場合は、Nbed のスクリプトと重複しないよう注意が必要です。

### クロックを変更する

既定では、NUCLEO-F446RE の ST-LINK MCO から供給される 8 MHz HSE を使い、180 MHz のシステム
クロックに設定します。別基板では発振器や分周値に合わせて `Clock::Configuration` を渡します。

```cpp
Clock::Configuration clock{};
clock.oscillator_source = Clock::OscillatorSource::kHse;
clock.hse_mode = Clock::HseMode::kCrystal;
clock.hse_frequency_hz = 8'000'000;

System::Initialize(clock);
```

PWM と CAN の実際の周波数はクロック設定に依存します。クロックを変更したら、通信速度・PWM の
周波数・実機上のタイミングを必ず再確認してください。

## 制約と未対応機能

- 対象 MCU は現在 STM32F446RE のみです。ほかの STM32F4 や F3/F7 向けのリンカスクリプトが
  リポジトリ内にあっても、このライブラリの公開 API と自動設定は F446RE 用です。
- SPI、外部割り込み、ADC DMA、UART DMA はこの API では提供していません。
- `Pwm::SendDataDma()` は将来の拡張用 API です。現版ではデータを出力しないため、使用しないで
  ください。
- I²C、UART の通常送受信、CAN の `Receive()` はブロッキングです。周期処理を止めたくない場合は
  タイムアウト設計や受信割り込みを検討してください。
- UART/CAN の受信コールバックは割り込み処理です。共有データをメインループで読む場合は、必要に
  応じて `volatile`、排他、コピーの一貫性を考慮してください。

## コントリビューター

- Yunoshin Tani — 初期開発・メンテナンス

## ライセンス

このプロジェクトは [MIT License](LICENSE) のもとで公開されています。利用、改変、再配布の条件は
[`LICENSE`](LICENSE) を参照してください。

## バージョン履歴

- v0.1.0 (2026-10-05) : 初版公開
- v0.2.0 (2026-10-06) : Arduino互換ピンへの対応
- v0.2.1 (2026-10-07) : nbed.h が include されている場合の clangd の補完の不具合を修正
