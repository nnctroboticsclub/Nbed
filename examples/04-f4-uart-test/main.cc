#include "logger.h"
#include "nbed.h"

// platformio.ini にて NBED_NO_DEFAULT_SERIAL を定義しているため、
// System::Initialize() 内でのデフォルト UART 初期化は行われない。

int main() {
  System::Initialize();

  Uart pc(kUsart2, kPA3, kPA2, 921600);
  pc.Initialize();
  pc.SetConsole();

  Logger::Info("test", "nbed F446RE UART test started");

  pc.SetRxCallback([&pc](UartRxMessage message) { pc.Send(message); });
  pc.EnableRxInterrupt();

  while (true) {
    SleepFor(1s);
  }
}
