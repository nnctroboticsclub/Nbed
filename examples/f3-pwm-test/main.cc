#include "nbed.h"

namespace {
void Require(bool success) {
  if (!success) {
    System::ErrorHandler();
  }
}
}  // namespace

int main() {
  // This environment disables the default USART2 console: PA2 is PWM here.
  System::Initialize();
  DigitalOut led(kD13);
  Pwm tim15_ch1(kTim15, kCh1, kPA2, 1'000);
  Pwm tim15_ch2(kTim15, kCh2, kPA3, 1'000);
  Pwm tim16_ch1(kTim16, kCh1, kPB4, 2'000);
  Pwm tim17_ch1(kTim17, kCh1, kPB5, 4'000);
  Require(led.Initialize());
  Require(tim15_ch1.Initialize() && tim15_ch2.Initialize());
  Require(tim16_ch1.Initialize() && tim17_ch1.Initialize());
  Require(tim15_ch1.SetDutyCycle(0.25F) && tim15_ch2.SetDutyCycle(0.75F));
  Require(tim16_ch1.SetDutyCycle(0.50F) && tim17_ch1.SetDutyCycle(0.50F));
  while (true) {
    led = !led.Read();
    SleepFor(500ms);
  }
}
