#include <cassert>
#include <chrono>
#include <limits>

#include "f7/clock.h"
#include "f7_hal_stubs.h"

int main() {
  using nkrc::nbed::stm32::f7::Clock;
  Clock::Configuration config{};
  assert(Clock::Initialize(config));
  assert(Clock::SystemClockHz() == 216'000'000);
  assert(Clock::HclkHz() == 216'000'000);
  assert(Clock::Pclk1Hz() == 54'000'000 && Clock::Pclk2Hz() == 108'000'000);
  assert(last_osc.HSEState == RCC_HSE_BYPASS);
  assert(last_flash_latency == 7 && overdrive_calls == 1);
  assert(timer_prescaler == RCC_TIMPRES_DESACTIVATED);
  const auto calls = osc_calls;
  config.apb1_divider = RCC_HCLK_DIV2;
  assert(!Clock::Initialize(config) && osc_calls == calls);
  config = {};
  config.apb2_divider = RCC_HCLK_DIV1;
  assert(!Clock::Initialize(config));
  config = {};
  config.flash_latency = 6;
  assert(!Clock::Initialize(config));
  config.flash_latency = 8;
  assert(!Clock::Initialize(config));
  config = {};
  config.hse_frequency_hz = 12'000'000;
  assert(!Clock::Initialize(config));
  config = {};
  config.pll_m = 0;
  assert(!Clock::Initialize(config));
  config = {};
  config.pll_m = 4;  // VCO would be 864 MHz.
  assert(!Clock::Initialize(config));
  config = {};
  config.hse_mode = static_cast<Clock::HseMode>(255);
  assert(!Clock::Initialize(config));

  config = {};
  config.oscillator_source = Clock::OscillatorSource::kHsi;
  config.pll_m = 16;
  assert(Clock::Initialize(config));
  assert(last_osc.HSEState == RCC_HSE_OFF && Clock::HclkHz() == 216'000'000);
  config.pll_n = 360;
  const auto overdrives = overdrive_calls;
  assert(Clock::Initialize(config) && Clock::HclkHz() == 180'000'000);
  assert(overdrive_calls == overdrives && last_flash_latency == 5);
  osc_status = HAL_ERROR;
  assert(!Clock::Initialize(config));
  osc_status = HAL_OK;
  config = {};
  overdrive_status = HAL_ERROR;
  assert(!Clock::Initialize(config));
  overdrive_status = HAL_OK;
  clock_status = HAL_ERROR;
  assert(!Clock::Initialize(config));
  clock_status = HAL_OK;
  config.ahb_divider = RCC_SYSCLK_DIV2;
  assert(Clock::Initialize(config) && Clock::HclkHz() == 108'000'000);

  Clock clock;
  clock.Sleep(std::chrono::milliseconds{-1});
  clock.Sleep(0);
  assert(delay_calls.empty());
  clock.Sleep(std::numeric_limits<uint32_t>::max());
  assert((delay_calls == std::vector<uint32_t>{0xfffffffeU, 1}));
}
