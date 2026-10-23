#include "nbed.h"

int main() {
  System::Initialize();

  DigitalOut led(kD13);
  if (!led.Initialize()) {
    System::ErrorHandler();
  }

  while (true) {
    led = true;
    SleepFor(500ms);
    led = false;
    SleepFor(500ms);
  }
}
