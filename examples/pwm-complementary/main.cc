#include "nbed.h"

int main() {
  System::Initialize();
#if defined(STM32F303x8)
  Pwm output(kTim15, kCh1, kPA2, 20'000,
             {.complementary_pin = kPA1, .dead_time = 8});
  DigitalOut led(kD13);
#elif defined(STM32F767xx)
  // PE9/PE8 avoid PA7, which is shared with the Ethernet PHY on this board.
  Pwm output(kTim1, kCh1, kPE9, 20'000,
             {.complementary_pin = kPE8, .dead_time = 8});
  DigitalOut led(kPB0);
#else
  Pwm output(kTim1, kCh1, kPA8, 20'000,
             {.complementary_pin = kPA7, .dead_time = 8});
  DigitalOut led(kD13);
#endif
  if (!output.Initialize() || !output.SetDutyCycle(0.25F) ||
      !led.Initialize()) {
    System::ErrorHandler();
  }
  while (true) {
    led = !led.Read();
    SleepFor(500ms);
  }
}
