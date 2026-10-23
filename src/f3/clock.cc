#if defined(STM32F303x8)

#include "f3/clock.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>

#include "f3/time.h"
#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f3 {
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

}  // namespace

void Clock::Initialize() { (void)Initialize(Configuration{}); }

bool Clock::Initialize(const Configuration& configuration) {
  const bool hse = configuration.oscillator_source == OscillatorSource::kHse;
  const auto ahb = AhbDividerFactor(configuration.ahb_divider);
  const auto apb1 = ApbDividerFactor(configuration.apb1_divider);
  const auto apb2 = ApbDividerFactor(configuration.apb2_divider);
  if ((!hse && configuration.oscillator_source != OscillatorSource::kHsi) ||
      !ahb || !apb1 || !apb2 || configuration.pll_multiplier < 2 ||
      configuration.pll_multiplier > 16 || configuration.hse_predivider < 1 ||
      configuration.hse_predivider > 16 ||
      (hse && (configuration.hse_frequency_hz != HSE_VALUE ||
               configuration.hse_frequency_hz < 4'000'000U ||
               configuration.hse_frequency_hz > 32'000'000U))) {
    return false;
  }
  const uint64_t source = hse ? configuration.hse_frequency_hz : HSI_VALUE;
  const uint64_t divisor = hse ? configuration.hse_predivider : 2U;
  const uint64_t numerator = source * configuration.pll_multiplier;
  if (source < divisor * 1'000'000U || source > divisor * 24'000'000U ||
      numerator < divisor * 16'000'000U || numerator > divisor * 72'000'000U ||
      numerator > divisor * *ahb * *apb1 * 36'000'000U ||
      numerator > divisor * *ahb * *apb2 * 72'000'000U) {
    return false;
  }
  const uint32_t hclk = static_cast<uint32_t>(numerator / divisor / *ahb);
  const uint32_t minimum_latency = hclk > 48'000'000U   ? FLASH_LATENCY_2
                                   : hclk > 24'000'000U ? FLASH_LATENCY_1
                                                        : FLASH_LATENCY_0;
  const uint32_t latency = configuration.flash_latency == 0xffffffffU
                               ? minimum_latency
                               : configuration.flash_latency;
  if (latency < minimum_latency || latency > FLASH_LATENCY_2) {
    return false;
  }
  RCC_OscInitTypeDef osc{};
  osc.OscillatorType =
      RCC_OSCILLATORTYPE_HSI | (hse ? RCC_OSCILLATORTYPE_HSE : 0U);
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.HSEState =
      configuration.hse_mode == HseMode::kBypass ? RCC_HSE_BYPASS : RCC_HSE_ON;
  osc.HSEPredivValue = configuration.hse_predivider - 1U;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = hse ? RCC_PLLSOURCE_HSE : RCC_PLLSOURCE_HSI;
  osc.PLL.PLLMUL = (configuration.pll_multiplier - 2U) << RCC_CFGR_PLLMUL_Pos;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    return false;
  }
  RCC_ClkInitTypeDef clk{};
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = configuration.ahb_divider;
  clk.APB1CLKDivider = configuration.apb1_divider;
  clk.APB2CLKDivider = configuration.apb2_divider;
  return HAL_RCC_ClockConfig(&clk, latency) == HAL_OK;
}

void Clock::Sleep(uint32_t milliseconds) {
  Sleep(std::chrono::milliseconds{milliseconds});
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

uint32_t Clock::SystemClockHz() { return HAL_RCC_GetSysClockFreq(); }

uint32_t Clock::HclkHz() { return HAL_RCC_GetHCLKFreq(); }

uint32_t Clock::Pclk1Hz() { return HAL_RCC_GetPCLK1Freq(); }

uint32_t Clock::Pclk2Hz() { return HAL_RCC_GetPCLK2Freq(); }

constinit Clock ClockManager::clock_{};

bool ClockManager::Initialize() {
  return Clock::Initialize(Clock::Configuration{});
}

bool ClockManager::Initialize(const Clock::Configuration& configuration) {
  return Clock::Initialize(configuration);
}

Clock& ClockManager::GetClock() { return clock_; }

extern "C" {

void SysTick_Handler(void) {
  HAL_IncTick();
  Time::OnSysTick();
}

}  // extern "C"

}  // namespace nkrc::nbed::stm32::f3

#endif
