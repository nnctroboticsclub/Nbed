#pragma once
// IWYU pragma: private, include "nbed.h"

#include <chrono>
#include <cstdint>

#include "interface/clock.h"
#include "stm32f3xx_hal.h"  // IWYU pragma: export
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {

class Clock final : public interface::Clock {
 public:
  enum class OscillatorSource : uint8_t {
    kHsi,
    kHse,
  };

  enum class HseMode : uint8_t {
    kCrystal,
    kBypass,
  };

  struct Configuration {
    // HSI/2 * 16 = 64 MHz; no external oscillator is required.
    OscillatorSource oscillator_source{OscillatorSource::kHsi};
    HseMode hse_mode{HseMode::kCrystal};
    uint32_t hse_frequency_hz{HSE_VALUE};
    uint32_t pll_multiplier{16};  // Integer multiplier, 2..16.
    uint32_t hse_predivider{1};   // Integer divisor, 1..16 (HSE only).
    uint32_t ahb_divider{RCC_SYSCLK_DIV1};
    uint32_t apb1_divider{RCC_HCLK_DIV2};
    uint32_t apb2_divider{RCC_HCLK_DIV1};
    uint32_t flash_latency{0xffffffffU};
  };

  void Initialize() override;
  [[nodiscard]] static bool Initialize(const Configuration& configuration);

  void Sleep(uint32_t milliseconds) override;
  void Sleep(std::chrono::milliseconds milliseconds) override;

  [[nodiscard]] static uint32_t SystemClockHz();
  [[nodiscard]] static uint32_t HclkHz();
  [[nodiscard]] static uint32_t Pclk1Hz();
  [[nodiscard]] static uint32_t Pclk2Hz();
};

class ClockManager {
 public:
  ClockManager() = delete;

  [[nodiscard]] static bool Initialize();
  [[nodiscard]] static bool Initialize(
      const Clock::Configuration& configuration);

  static Clock& GetClock();

 private:
  static constinit Clock clock_;
};

}  // namespace nkrc::nbed::stm32::f3
