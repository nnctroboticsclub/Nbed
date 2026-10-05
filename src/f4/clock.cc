#if defined(STM32F446xx)

#include "f4/clock.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_pwr_ex.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/time.h"

namespace nbed::f4 {
namespace {

std::optional<uint32_t> AhbDividerFactor(uint32_t divider) {
  switch (divider) {
    case RCC_SYSCLK_DIV1:
      return 1;
    case RCC_SYSCLK_DIV2:
      return 2;
    case RCC_SYSCLK_DIV4:
      return 4;
    case RCC_SYSCLK_DIV8:
      return 8;
    case RCC_SYSCLK_DIV16:
      return 16;
    case RCC_SYSCLK_DIV64:
      return 64;
    case RCC_SYSCLK_DIV128:
      return 128;
    case RCC_SYSCLK_DIV256:
      return 256;
    case RCC_SYSCLK_DIV512:
      return 512;
    default:
      return std::nullopt;
  }
}

std::optional<uint32_t> ApbDividerFactor(uint32_t divider) {
  switch (divider) {
    case RCC_HCLK_DIV1:
      return 1;
    case RCC_HCLK_DIV2:
      return 2;
    case RCC_HCLK_DIV4:
      return 4;
    case RCC_HCLK_DIV8:
      return 8;
    case RCC_HCLK_DIV16:
      return 16;
    default:
      return std::nullopt;
  }
}

bool IsValidConfiguration(const Clock::Configuration& configuration) {
  constexpr uint64_t kHsiFrequencyHz = 16'000'000ULL;
  constexpr uint64_t kMinimumHseFrequencyHz = 4'000'000ULL;
  constexpr uint64_t kMaximumHseFrequencyHz = 26'000'000ULL;
  constexpr uint64_t kMinimumPllInputHz = 1'000'000ULL;
  constexpr uint64_t kMaximumPllInputHz = 2'000'000ULL;
  constexpr uint64_t kMinimumPllVcoHz = 100'000'000ULL;
  constexpr uint64_t kMaximumPllVcoHz = 432'000'000ULL;
  constexpr uint64_t kMaximumSystemClockHz = 180'000'000ULL;
  constexpr uint64_t kMaximumApb1ClockHz = 45'000'000ULL;
  constexpr uint64_t kMaximumApb2ClockHz = 90'000'000ULL;

  if (!IS_RCC_PLLM_VALUE(configuration.pll_m) ||  // NOLINT
      configuration.pll_n < 50U || configuration.pll_n > 432U ||
      !IS_RCC_PLLP_VALUE(configuration.pll_p) ||  // NOLINT
      !IS_RCC_PLLQ_VALUE(configuration.pll_q)) {  // NOLINT
    return false;
  }
  const auto ahb_divider = AhbDividerFactor(configuration.ahb_divider);
  const auto apb1_divider = ApbDividerFactor(configuration.apb1_divider);
  const auto apb2_divider = ApbDividerFactor(configuration.apb2_divider);
  if (!ahb_divider || !apb1_divider || !apb2_divider) {
    return false;
  }

  const uint64_t source_hz =
      configuration.oscillator_source == Clock::OscillatorSource::kHse
          ? static_cast<uint64_t>(configuration.hse_frequency_hz)
          : kHsiFrequencyHz;
  if ((configuration.oscillator_source == Clock::OscillatorSource::kHse &&
       (source_hz < kMinimumHseFrequencyHz ||
        source_hz > kMaximumHseFrequencyHz)) ||
      source_hz <
          static_cast<uint64_t>(configuration.pll_m) * kMinimumPllInputHz ||
      source_hz >
          static_cast<uint64_t>(configuration.pll_m) * kMaximumPllInputHz) {
    return false;
  }

  const uint64_t vco_numerator = source_hz * configuration.pll_n;
  const uint64_t pll_m = configuration.pll_m;
  if (vco_numerator < pll_m * kMinimumPllVcoHz ||
      vco_numerator > pll_m * kMaximumPllVcoHz) {
    return false;
  }

  const uint64_t system_denominator = pll_m * configuration.pll_p;
  // clang-format off
  return !(vco_numerator > system_denominator * kMaximumSystemClockHz || // NOLINT
           vco_numerator > system_denominator * *ahb_divider * kMaximumSystemClockHz ||
           vco_numerator > system_denominator * *ahb_divider * *apb1_divider * kMaximumApb1ClockHz ||
           vco_numerator > system_denominator * *ahb_divider * *apb2_divider * kMaximumApb2ClockHz);
  // clang-format on
}

}  // namespace

void Clock::Initialize() {
  (void)Initialize(Configuration{});
}

bool Clock::Initialize(const Configuration& configuration) {  // NOLINT
  if (!IsValidConfiguration(configuration)) {
    return false;
  }

  RCC_OscInitTypeDef osc{};
  RCC_ClkInitTypeDef clk{};
  __HAL_RCC_PWR_CLK_ENABLE();  // NOLINT
  osc.OscillatorType = configuration.oscillator_source == OscillatorSource::kHse
                           ? RCC_OSCILLATORTYPE_HSE
                           : RCC_OSCILLATORTYPE_HSI;
  if (configuration.oscillator_source != OscillatorSource::kHse) {
    osc.HSEState = RCC_HSE_OFF;
  } else if (configuration.hse_mode == HseMode::kBypass) {
    osc.HSEState = RCC_HSE_BYPASS;
  } else {
    osc.HSEState = RCC_HSE_ON;
  }
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = configuration.oscillator_source == OscillatorSource::kHse
                          ? RCC_PLLSOURCE_HSE
                          : RCC_PLLSOURCE_HSI;
  osc.PLL.PLLM = configuration.pll_m;
  osc.PLL.PLLN = configuration.pll_n;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  if (configuration.pll_p == 4) {
    osc.PLL.PLLP = RCC_PLLP_DIV4;
  } else if (configuration.pll_p == 6) {
    osc.PLL.PLLP = RCC_PLLP_DIV6;
  } else if (configuration.pll_p == 8) {
    osc.PLL.PLLP = RCC_PLLP_DIV8;
  }
  osc.PLL.PLLQ = configuration.pll_q;
  const uint64_t source_hz =
      configuration.oscillator_source == OscillatorSource::kHse
          ? static_cast<uint64_t>(configuration.hse_frequency_hz)
          : 16'000'000ULL;
  const uint32_t system_clock_hz =
      static_cast<uint32_t>(source_hz * configuration.pll_n /
                            configuration.pll_m / configuration.pll_p);
  uint32_t minimum_flash_latency = system_clock_hz / 30'000'000U;
  minimum_flash_latency =
      std::min(minimum_flash_latency, static_cast<uint32_t>(FLASH_LATENCY_5));
  if (configuration.flash_latency != 0xffffffffU &&
      (configuration.flash_latency < minimum_flash_latency ||
       configuration.flash_latency > FLASH_LATENCY_5)) {
    return false;
  }
  const uint32_t flash_latency = configuration.flash_latency == 0xffffffffU
                                     ? minimum_flash_latency
                                     : configuration.flash_latency;
  uint32_t voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE3;
  if (system_clock_hz > 144'000'000U) {
    voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE1;
  } else if (system_clock_hz > 120'000'000U) {
    voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE2;
  }
  __HAL_PWR_VOLTAGESCALING_CONFIG(voltage_scale);  // NOLINT

  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    return false;
  }
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = configuration.ahb_divider;
  clk.APB1CLKDivider = configuration.apb1_divider;
  clk.APB2CLKDivider = configuration.apb2_divider;
  return HAL_RCC_ClockConfig(&clk, flash_latency) == HAL_OK;
}

void Clock::Sleep(uint32_t milliseconds) {
  HAL_Delay(milliseconds);
}

void Clock::Sleep(std::chrono::milliseconds milliseconds) {
  if (milliseconds <= std::chrono::milliseconds::zero()) {
    return;
  }

  // HAL_Delay() adds one tick internally, so UINT32_MAX would wrap there.
  constexpr uint32_t kLargestSafeDelay =
      std::numeric_limits<uint32_t>::max() - 1U;
  auto remaining = static_cast<uint64_t>(milliseconds.count());
  while (remaining > kLargestSafeDelay) {
    HAL_Delay(kLargestSafeDelay);
    remaining -= kLargestSafeDelay;
  }
  HAL_Delay(static_cast<uint32_t>(remaining));
}

uint32_t Clock::SystemClockHz() {
  return HAL_RCC_GetSysClockFreq();
}

uint32_t Clock::HclkHz() {
  return HAL_RCC_GetHCLKFreq();
}

uint32_t Clock::Pclk1Hz() {
  return HAL_RCC_GetPCLK1Freq();
}

uint32_t Clock::Pclk2Hz() {
  return HAL_RCC_GetPCLK2Freq();
}

constinit Clock ClockManager::clock_{};

bool ClockManager::Initialize() {
  return Clock::Initialize(Clock::Configuration{});
}

bool ClockManager::Initialize(const Clock::Configuration& configuration) {
  return Clock::Initialize(configuration);
}

Clock& ClockManager::GetClock() {
  return clock_;
}

extern "C" {

void SysTick_Handler(void) {  // NOLINT
  HAL_IncTick();
  Time::OnSysTick();
}

}  // extern "C"

}  // namespace nbed::f4

#endif
