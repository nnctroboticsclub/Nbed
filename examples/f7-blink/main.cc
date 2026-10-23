#include "nbed.h"

int main() {
  System::Initialize();
  DigitalOut led(kPB0);  // NUCLEO-F767ZI LD1 (green), not Arduino D13.
  if (!led.Initialize()) {
    System::ErrorHandler();
  }
  while (true) {
    led = !led.Read();
    SleepFor(500ms);
  }
}
