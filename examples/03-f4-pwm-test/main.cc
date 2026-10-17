#include "nbed.h"

#include "logger.h"

int main() {
  System::Initialize();

  Pwm pwm(kTim3, kCh1, kPB4, 50);

  if (!pwm.Initialize()) {
    Logger::Error("test", "PWM initialization failed");
    System::ErrorHandler();
  }

  while (true) {
    pwm.SetPulseWidthUs(1000);
    SleepFor(1s);
    pwm.SetPulseWidthUs(1500);
    SleepFor(1s);
    pwm.SetPulseWidthUs(2000);
    SleepFor(1s);
  }
}
