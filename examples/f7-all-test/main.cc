#include <array>
#include <cstdio>

#include "nbed.h"

namespace {
void Require(bool success) {
  if (!success) {
    System::ErrorHandler();
  }
}
}  // namespace

int main() {
  System::Initialize();  // USART3 PD9/PD8 console, 216 MHz, I-cache enabled.
  DigitalOut led(kPB0);
  DigitalIn button(kPC13, GpioPull::kDown);
  Require(led.Initialize() && button.Initialize());
  AnalogIn adc1(kA0);
  AnalogIn adc3(kA3);
  Require(adc1.Initialize() && adc3.Initialize());
  {
    Pwm pwm(kTim1, kCh1, kPE9, 1'000);
    Require(pwm.Initialize() && pwm.SetDutyCycle(0.25F));
    Require(pwm.SetFrequencyHz(2'000) && pwm.SetPulseWidthUs(100));
    pwm.Stop();
  }
  Encoder encoder(EncoderTimer::kTim1, kPE9, kPE11, 2048);
  Require(encoder.Initialize());
  encoder.ResetCount();
  I2c i2c(kI2c1, kPB8, kPB9, 100'000);
  Require(i2c.Initialize());
  Uart uart(kUsart6, kD0, kD1, 115'200);
  Require(uart.Initialize());
  uart.SetRxCallback([](UartRxMessage) {});
  Require(uart.EnableRxInterrupt());
  Can can({.peripheral = kCan1,
           .rx = kPD0,
           .tx = kPD1,
           .bitrate = 1'000'000,
           .mode = kLoopback});
  Require(can.Initialize() && can.SetRxFilter(0x123, 0x7ff) && can.Start());
  const CanMessage tx{.id = 0x123, .size = 3, .data = {0x4e, 0x42, 0x45}};
  Require(can.Send(tx));
  Timer timeout;
  timeout.Start();
  CanMessage rx{};
  while (!can.Receive(rx)) {
    Require(timeout.ElapsedMicroseconds() < 100'000);
  }
  Require(rx.id == tx.id && rx.size == tx.size && rx.data == tx.data);
  can.SetRxCallback([](const CanMessage&) {});
  Require(can.EnableRxInterrupt());
  printf("F767ZI: %lu Hz, CAN loopback PASS\n",
         static_cast<unsigned long>(Clock::SystemClockHz()));
  while (true) {
    led = !led.Read();
    printf("ADC1=%.3f V ADC3=%.3f V count=%d button=%d\n",
           static_cast<double>(adc1.ReadVoltage()),
           static_cast<double>(adc3.ReadVoltage()), encoder.GetCount(),
           static_cast<int>(button.Read()));
    SleepFor(500ms);
  }
}
