#pragma once
// IWYU pragma: private, include "nbed.h"

#include <chrono>
#include <cstdint>

#include "interface/clock.h"
#include "stm32f7xx_hal.h"  // IWYU pragma: export
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {

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
    OscillatorSource oscillator_source{OscillatorSource::kHse};
    // NUCLEO-F767ZI receives its 8 MHz HSE clock from ST-LINK MCO.
    HseMode hse_mode{HseMode::kBypass};
    uint32_t hse_frequency_hz{8'000'000};
    uint32_t pll_m{8};
    uint32_t pll_n{432};
    uint32_t pll_p{2};
    uint32_t pll_q{9};
    uint32_t ahb_divider{RCC_SYSCLK_DIV1};
    uint32_t apb1_divider{RCC_HCLK_DIV4};
    uint32_t apb2_divider{RCC_HCLK_DIV2};
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

}  // namespace nkrc::nbed::stm32::f7
