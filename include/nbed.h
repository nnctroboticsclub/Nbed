#pragma once

// C++ 標準ライブラリ
#include <chrono>
#include <cstdint>

// 単位系
#include "units.h"  // IWYU pragma: export

// インターフェース
#include "interface/can.h"        // IWYU pragma: export
#include "interface/encoder.h"    // IWYU pragma: export
#include "interface/i2c.h"        // IWYU pragma: export
#include "interface/pin_names.h"  // IWYU pragma: export
#include "interface/uart.h"       // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using nbed::interface::CanFrameType;
using nbed::interface::CanMessage;
using nbed::interface::CanRxCallback;
using nbed::interface::CanRxFilter;
using nbed::interface::I2CMessage;
using nbed::interface::UartRxCallback;
using nbed::interface::UartRxMessage;
using nbed::interface::UartTxMessage;
using enum nbed::interface::CanFormat;
using enum nbed::interface::CanFrameType;
using enum nbed::interface::PinName;
using namespace nbed::literals;
using namespace std::chrono_literals;
#endif

// STM32F4シリーズ用の実装
#ifdef STM32F446xx
#include "f4/analog_in.h"  // IWYU pragma: export
#include "f4/can.h"        // IWYU pragma: export
#include "f4/clock.h"      // IWYU pragma: export
#include "f4/encoder.h"    // IWYU pragma: export
#include "f4/gpio.h"       // IWYU pragma: export
#include "f4/i2c.h"        // IWYU pragma: export
#include "f4/pwm.h"        // IWYU pragma: export
#include "f4/system.h"     // IWYU pragma: export
#include "f4/time.h"       // IWYU pragma: export
#include "f4/uart.h"       // IWYU pragma: export

// 初心者向けグローバル名前空間
#ifndef NBED_NO_GLOBAL_NAMES
using namespace nbed::f4;
using enum nbed::f4::CanPeripheral;
using enum nbed::f4::CanMode;
// using enum nbed::f4::EncoderTimer;
using enum nbed::f4::GpioPull;
using enum nbed::f4::I2cPeripheral;
using enum nbed::f4::PwmTimer;
using enum nbed::f4::PwmChannel;
using enum nbed::f4::UartPeripheral;
#endif  // NBED_NO_GLOBAL_NAMES

#ifndef NBED_NO_GLOBAL_FUNCTIONS
inline void SleepFor(std::chrono::seconds sec) {
  nbed::f4::ClockManager::GetClock().Sleep(
      std::chrono::duration_cast<std::chrono::milliseconds>(sec));
}

inline void SleepFor(std::chrono::milliseconds ms) {
  nbed::f4::ClockManager::GetClock().Sleep(ms);
}

inline void SleepFor(uint32_t ms) {
  nbed::f4::ClockManager::GetClock().Sleep(ms);
}
#endif  // NBED_NO_GLOBAL_FUNCTIONS
#endif  // STM32F446xx
