#include <cstdio>  // printf用

#include "nbed.h"  // nbedライブラリ (この中に複数のヘッダーファイルが含まれている)

// ---------- デジタル入出力の例 ----------
void DigitalExample() {
  // クロックや割り込みなどの設定
  System::Initialize();  // マイコンの初期化

  // 定義するのみで、まだマイコンには設定されない
  DigitalOut led(kPA5);     // LEDを定義
  DigitalIn button(kPC13);  // ボタンを定義

  // ピンの情報などをマイコンに設定
  led.Initialize();     // LEDを初期化
  button.Initialize();  // ボタンを初期化

  led.Write(false);  // LEDを消灯

  constexpr bool kIsButtonPressed = false;  // Nucleoのボタンは押されたらLOW

  while (true) {
    if (button.Read() == kIsButtonPressed) {
      led.Write(true);  // ボタンが押されたらLEDを点灯
    } else {
      led.Write(false);  // ボタンが離されたらLEDを消灯
    }

    // 以下のようにDigitalOutとDigitalInの代入演算子を使うこともできる
    // led = button == kIsButtonPressed;

    printf("Button state: %d\n", static_cast<int>(button.Read()));
  }
}

// ---------- アナログ入力の例 ----------
void AnalogExample() {
  System::Initialize();

  AnalogIn analog(kPA0);  // アナログ入力を定義
  analog.Initialize();    // アナログ入力を初期化

  while (true) {
    const float voltage = analog.ReadVoltage();  // 電圧を読み取る
    printf("Voltage: %.3f V\n", voltage);
    SleepFor(100ms);  // 100ms待機
  }
}

// ---------- PWMの例 ----------
void PwmExample() {
  System::Initialize();

  // PWMを定義 (タイマ3, チャネル1, ピンPB4, 周波数50Hz)
  Pwm pwm(kTim3, kCh1, kPB4, 50);

  // ほぼすべての設定用関数は設定出来たらtrue、失敗したらfalseを返す (デバッグに活用できる)
  if (!pwm.Initialize()) {
    printf("PWM initialization failed\n");
    System::ErrorHandler();  // エラーを知らせる関数、好みの処理にしてもよい
  }

  while (true) {
    pwm.SetPulseWidthUs(1000);  // パルス幅を1000usに設定
    SleepFor(1s);               // 1秒待機

    pwm.SetDutyCycle(0.5f);  // デューティ比を50%に設定
    SleepFor(1s);            // 1秒待機
  }
}

// ---------- CAN通信の例 ----------

// CANの受信コールバック関数 (CANが受信をした際に自動で実行するための関数)
// can.SetRxCallback(OnRxCallback); のように設定することで、CANが受信した際に自動で呼ばれる
// if (can.Receive(message)) という従来の方法だと受信を逃すことがあるため、受信割り込みを使う方法が推奨される
void OnRxCallback(const CanMessage& message) {
  printf("Received CAN message! Data: ");
  for (size_t i = 0; i < message.size; ++i) {
    printf("%02X, ", message.data.at(i));
  }
  printf("\n");
}

// Can1で送信したCANメッセージをCan2で受信する
// Can1とCan2をケーブルで接続すれば、Can1で送信したCANメッセージをCan2で受信することができる
void CanExample() {
  System::Initialize();

  // CAN1を定義 (CAN1, RX:PB8, TX:PB9, ボーレート1Mbps)
  Can can1(kCan1, kPB8, kPB9, 1'000'000);

  // CAN2を定義 (CAN2, RX:PB5, TX:PB6, ボーレート1Mbps)
  Can can2(kCan2, kPB5, kPB6, 1'000'000);

  can1.Initialize();  // CAN1を初期化
  can2.Initialize();  // CAN2を初期化

  can2.SetRxCallback(OnRxCallback);  // CANの受信コールバック関数を登録

  can2.EnableRxInterrupt();  // CANの受信割り込みを有効化

  can1.Start();  // CAN通信の開始
  can2.Start();  // CAN通信の開始

  // 送信するCANメッセージを定義
  CanMessage tx{
      .id = 0x123,
      .size = 8,
      .data = {0x4E, 0x42, 0x45, 0x44, 0x20, 0x43, 0x41, 0x4E},
  };

  // 以下のようにしてもよい
  CanMessage tx2;
  tx2.id = 0x456;
  tx2.size = 8;
  tx2.data = {0x4E, 0x42, 0x45, 0x44, 0x20, 0x43, 0x41, 0x4E};

  while (true) {
    can1.Send(tx);    // CAN1でCANメッセージを送信
    SleepFor(100ms);  // 100ms待機
  }
}

// ---------- メイン関数 ----------
int main() {
  // DigitalExample();
  // AnalogExample();
  // PwmExample();
  CanExample();

  return 0;
}

// ---------- その他 ----------
// デフォルトでprintfはUSART2に出力されるようになっている
// その他初心者向け機能はマクロを定義することで無効化できる
