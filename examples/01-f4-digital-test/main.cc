#include <cstdio>

#include "nbed.h"

int main() {
  System::Initialize();

  DigitalOut led(kD13);
  DigitalIn button(kPC13);

  led.Initialize();
  button.Initialize();

  while (true) {
    led = !button;
    printf("Button state: %d\n", static_cast<int>(button.Read()));
  }
}
