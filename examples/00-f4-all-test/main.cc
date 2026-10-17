#include <array>
#include <cstdint>

#include "logger.h"
#include "nbed.h"

bool GpioTest(DigitalOut& led, DigitalIn& button) {
  const bool initialized = led.Initialize() && button.Initialize();
  Logger::Info("test", "GPIO initialize: %s", initialized ? "PASS" : "FAIL");

  led.Write(true);
  const bool read = led.Read();
  Logger::Info("test", "GPIO read: %s", read ? "PASS" : "FAIL");
  led.Write(false);
  return initialized && read;
}

bool AnalogInTest(AnalogIn& analog) {
  const bool initialized = analog.Initialize();
  Logger::Info("test", "ADC initialize: %s", initialized ? "PASS" : "FAIL");
  if (initialized) {
    Logger::Value("adc", "ratio: %.3f, voltage: %.3f V", analog.ReadRatio(),
                  analog.ReadVoltage());
  }
  return initialized;
}

bool PwmTest(Pwm& pwm) {
  const bool initialized = pwm.Initialize();
  Logger::Info("test", "PWM initialize: %s", initialized ? "PASS" : "FAIL");
  const bool duty_cycle = pwm.SetDutyCycle(0.5F);
  Logger::Info("test", "PWM duty cycle: %s", duty_cycle ? "PASS" : "FAIL");
  const bool pulse_width = pwm.SetPulseWidthUs(1500);
  Logger::Info("test", "PWM pulse width: %s", pulse_width ? "PASS" : "FAIL");
  return initialized && duty_cycle && pulse_width;
}

bool UartTest(Uart& uart) {
  constexpr std::array<uint8_t, 16> kMessage{
      'N', 'b', 'e', 'd', ' ', 'U', 'A',  'R',
      'T', ' ', 't', 'e', 's', 't', '\r', '\n',
  };
  const bool sent = uart.Send(kMessage);
  Logger::Info("test", "UART send: %s", sent ? "PASS" : "FAIL");
  return sent;
}

bool I2cTest(I2c& i2c) {
  const bool initialized = i2c.Initialize();
  Logger::Info("test", "I2C initialize: %s", initialized ? "PASS" : "FAIL");
  return initialized;
}

bool CanTest(nkrc::nbed::interface::Can& can) {
  const bool initialized = can.Initialize();
  Logger::Info("test", "CAN initialize: %s", initialized ? "PASS" : "FAIL");
  const bool filter = can.SetRxFilter(CanRxFilter{
      .id = 0x123,
      .mask = 0x7ff,
      .format = nkrc::nbed::interface::CanFormat::kStandard,
  });
  Logger::Info("test", "CAN filter: %s", filter ? "PASS" : "FAIL");
  const bool started = can.Start();
  Logger::Info("test", "CAN start: %s", started ? "PASS" : "FAIL");
  const CanMessage tx{
      .id = 0x123,
      .size = 8,
      .data = {0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5, 0x06, 0x17},
  };
  const bool sent = can.Send(tx);
  Logger::Info("test", "CAN send: %s", sent ? "PASS" : "FAIL");
  CanMessage rx;
  const bool received = can.Receive(rx);
  Logger::Info("test", "CAN receive: %s", received ? "PASS" : "FAIL");
  if (received) {
    Logger::Value("can",
                  "received message(id:0x%X, size:%u, data:%02X %02X %02X "
                  "%02X %02X %02X %02X %02X,)",
                  rx.id, rx.size, rx.data.at(0), rx.data.at(1), rx.data.at(2),
                  rx.data.at(3), rx.data.at(4), rx.data.at(5), rx.data.at(6),
                  rx.data.at(7));
  }
  return initialized && filter && started && sent && received;
}

bool TimerTest(Timer& timer) {
  timer.Start();
  ClockManager::GetClock().Sleep(10);
  timer.Stop();

  const bool elapsed =
      !timer.IsRunning() && timer.ElapsedMicroseconds() >= 10'000;
  Logger::Info("test", "Timer elapsed: %s", elapsed ? "PASS" : "FAIL");
  Logger::Value("timer", "elapsed: %lu us",
                static_cast<unsigned long>(timer.ElapsedMicroseconds()));
  return elapsed;
}

// platformio.ini にて NBED_NO_DEFAULT_SERIAL を定義しているため、
// System::Initialize() 内でのデフォルト UART 初期化は行われない。

int main() {
  System::Initialize();

  Uart uart(kUsart2, kPA3, kPA2, 921600);
  const bool uart_initialized = uart.Initialize();
  if (uart_initialized) {
    uart.SetConsole();
  }

  bool passed = true;

  Logger::Info("test", "Nbed F446RE hardware test started");
  Logger::Value("clock", "SYSCLK: %lu, HCLK: %lu, PCLK1: %lu, PCLK2: %lu",
                static_cast<unsigned long>(Clock::SystemClockHz()),
                static_cast<unsigned long>(Clock::HclkHz()),
                static_cast<unsigned long>(Clock::Pclk1Hz()),
                static_cast<unsigned long>(Clock::Pclk2Hz()));

  DigitalOut led(kPA5);
  DigitalIn button(kPC13);
  passed = GpioTest(led, button) && passed;

  AnalogIn analog(kPA0);
  passed = AnalogInTest(analog) && passed;

  Pwm pwm(kTim3, kCh1, kPC6, 50);
  passed = PwmTest(pwm) && passed;

  Logger::Info("test", "UART initialize: %s",
               uart_initialized ? "PASS" : "FAIL");
  if (uart_initialized) {
    passed = UartTest(uart) && passed;
  } else {
    passed = false;
  }

  I2c i2c(kI2c1, kPB6, kPB7, 100'000);
  passed = I2cTest(i2c) && passed;

  Can can({
      .peripheral = kCan1,
      .rx = kPB8,
      .tx = kPB9,
      .bitrate = 1'000'000,
      .mode = kLoopback,
  });
  passed = CanTest(can) && passed;

  Timer timer;
  passed = TimerTest(timer) && passed;

  if (!passed) {
    Logger::Error("test", "Nbed hardware test failed");
    System::ErrorHandler();
  }

  Logger::Info("test", "Nbed hardware test passed");

  while (true) {
    led = !led.Read();

    if (!button.Read()) {
      SleepFor(100ms);
    } else {
      SleepFor(500ms);
    }
  }
}
