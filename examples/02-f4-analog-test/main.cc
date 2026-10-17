#include "logger.h"
#include "nbed.h"

int main() {
  System::Initialize();

  AnalogIn analog(kPA0);
  analog.Initialize();

  while (true) {
    const float voltage = analog.ReadVoltage();
    Logger::Value("analog", "voltage: %.3f V", voltage);
    SleepFor(100ms);
  }
}
