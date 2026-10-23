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
  System::Initialize();
  Uart console(kUsart2, kPA15, kPA2);
  Require(console.Initialize());
  console.SetConsole();
  DigitalOut led(kD13);
  DigitalIn input(kPA8, GpioPull::kUp);
  AnalogIn adc1(kA2);
  AnalogIn adc2(kA3);
  Require(led.Initialize() && input.Initialize());
  Require(adc1.Initialize() && adc2.Initialize());

  // TIM2 is released before the encoder claims it.
  {
    Pwm pwm(kTim2, kCh1, kPA0, 1'000);
    Require(pwm.Initialize() && pwm.SetDutyCycle(0.75F));
    Require(pwm.SetFrequencyHz(2'000) && pwm.SetPulseWidthUs(100));
    pwm.Stop();
  }
  Encoder encoder(EncoderTimer::kTim2, kPA0, kPA1, 2048);
  Require(encoder.Initialize());
  encoder.ResetCount();
  I2c i2c(kI2c1, kPB6, kPB7, 100'000);
  Require(i2c.Initialize());

  Can can({.peripheral = kCan1,
           .rx = kPA11,
           .tx = kPA12,
           .bitrate = 1'000'000,
           .mode = kLoopback});
  Require(can.Initialize() && can.SetRxFilter(0x123, 0x7ff) && can.Start());
  const CanMessage sent{.id = 0x123, .size = 3, .data = {0x4e, 0x42, 0x45}};
  Require(can.Send(sent));
  Timer timeout;
  timeout.Start();
  CanMessage received{};
  while (!can.Receive(received)) {
    Require(timeout.ElapsedMicroseconds() < 100'000);
  }
  Require(received.id == sent.id && received.size == sent.size &&
          received.data == sent.data);
  can.SetRxCallback([](const CanMessage&) {});
  Require(can.EnableRxInterrupt());
  console.SetRxCallback([](UartRxMessage) {});
  Require(console.EnableRxInterrupt());

  Timer timer;
  timer.Start();
  while (true) {
    led = input.Read();
    printf("adc1=%.3f adc2=%.3f count=%d angle=%.1f time=%lu ms\n",
           static_cast<double>(adc1.ReadVoltage()),
           static_cast<double>(adc2.ReadVoltage()), encoder.GetCount(),
           static_cast<double>(encoder.GetAngle().GetAsDegree()),
           static_cast<unsigned long>(timer.ElapsedMicroseconds() / 1000));
    SleepFor(500ms);
  }
}
