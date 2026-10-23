#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "detail/i2c_timing.h"
#include "f7/analog_in.h"
#include "f7/i2c.h"
#include "f7_hal_stubs.h"

constexpr bool CheckTimings(uint32_t first, uint32_t last) {
  for (uint32_t rate = first; rate <= last; rate += 137) {
    const auto timing = nkrc::nbed::detail::I2cTimingHsi16Mhz(rate);
    if (!timing) {
      return false;
    }
    const uint32_t tick = ((*timing >> 28) + 1) * 125;
    const uint32_t high = (((*timing >> 8) & 255) + 1) * tick + 350;
    const uint32_t low = ((*timing & 255) + 1) * tick + 350;
    const uint32_t setup = (((*timing >> 20) & 15) + 1) * tick;
    const uint32_t hold = ((*timing >> 16) & 15) * tick;
    const bool standard = rate <= 100'000;
    if (uint64_t{high + low} * rate < 2'000'000'000ULL ||
        high < (standard ? 8000U : 1200U) || low < (standard ? 9400U : 2600U) ||
        setup < (standard ? 2500U : 1000U)) {
      return false;
    }
    // Lower and upper SDA hold bounds from RM0410, in half-nanoseconds.
    if (hold + 100U + 3U * 125U < 600U ||
        hold + (standard ? 2000U : 200U) + 520U + 4U * 125U >
            (standard ? 6900U : 1800U)) {
      return false;
    }
  }
  return true;
}

static_assert(CheckTimings(2000, 100'000));
static_assert(CheckTimings(100'001, 400'000));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(0));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(1000));
static_assert(!nkrc::nbed::detail::I2cTimingHsi16Mhz(400'001));

int main() {
  using namespace nkrc::nbed::stm32::f7;
  using nkrc::nbed::interface::I2CMessage;
  using nkrc::nbed::interface::PinName;
  {
    I2c bus(I2cPeripheral::kI2c1, PinName::kPB8, PinName::kPB9);
    std::array<uint8_t, 2> data{};
    I2CMessage message{.address = 0x50, .size = 2, .data = data.data()};
    assert(!bus.Send(message));
    i2c_filter_status = HAL_ERROR;
    assert(!bus.Initialize() && i2c_deinits == 1);
    i2c_filter_status = HAL_OK;
    assert(bus.Initialize() && bus.Initialize());
    assert(last_i2c_init.Instance == I2C1 && i2c_clocks[0] == 1);
    assert(last_i2c_init.Init.Timing ==
           *nkrc::nbed::detail::I2cTimingHsi16Mhz(100'000));
    I2c duplicate(I2cPeripheral::kI2c1, PinName::kPB6, PinName::kPB7);
    assert(!duplicate.Initialize());
    assert(bus.Send(message) && bus.Receive(message) &&
           last_i2c_address == 0xa0);
    message.address = 0x80;
    assert(!bus.Send(message) && !bus.Receive(message));
    message.address = 0x50;
    message.data = nullptr;
    assert(!bus.Send(message));
    message.data = data.data();
    message.size = 0;
    assert(!bus.Receive(message));
    message.size = 2;
    i2c_transfer_status = HAL_ERROR;
    assert(!bus.Send(message) && !bus.Receive(message));
    i2c_transfer_status = HAL_OK;
  }
  I2c retry(I2cPeripheral::kI2c1, PinName::kPB6, PinName::kPB7, 400'000);
  assert(retry.Initialize());
  for (const auto scl : {PinName::kPB10, PinName::kPF1}) {
    for (const auto sda : {PinName::kPB11, PinName::kPF0}) {
      I2c bus(I2cPeripheral::kI2c2, scl, sda);
      assert(bus.Initialize() && last_i2c_init.Instance == I2C2 &&
             i2c_clocks[1]);
    }
  }
  I2c bus3(I2cPeripheral::kI2c3, PinName::kPA8, PinName::kPC9);
  assert(bus3.Initialize() && last_i2c_init.Instance == I2C3 && i2c_clocks[2]);
  I2c wrong(static_cast<I2cPeripheral>(255), PinName::kPB8, PinName::kPB9);
  assert(!wrong.Initialize());
  I2c wrong_pin(I2cPeripheral::kI2c2, PinName::kPB10, PinName::kPB3);
  assert(!wrong_pin.Initialize());

  AnalogIn a(PinName::kPB1), b(PinName::kPF3), c(PinName::kPF5);
  adc_init_status = HAL_ERROR;
  assert(!a.Initialize());
  adc_init_status = HAL_OK;
  assert(a.Initialize() && last_adc_init.Instance == ADC1);
  assert(last_adc_init.Init.ClockPrescaler == ADC_CLOCK_SYNC_PCLK_DIV4);
  assert(b.Initialize() && last_adc_init.Instance == ADC3 && c.Initialize());
  assert(a.ReadRatio() > 0.5F && b.ReadRatio() > 0.5F);
  assert(adc_channels.size() == 2);
  assert(adc_channels[0].first == ADC1 && adc_channels[0].second == 9);
  assert(adc_channels[1].first == ADC3 && adc_channels[1].second == 9);
  (void)a.ReadRatio();
  (void)b.ReadRatio();
  assert(adc_channels.size() == 2);  // Cache is independent for each ADC.
  assert(c.ReadRatio() > 0.5F && adc_channels.back().second == 15);
  assert(std::abs(a.ReadVoltage() - 2048.0F / 4095.0F * 3.3F) < 0.001F);
  const auto stops = adc_stops;
  adc_poll_status = HAL_ERROR;
  assert(a.ReadRatio() == 0 && adc_stops == stops + 1);
  adc_poll_status = HAL_OK;
  AnalogIn invalid(PinName::kPF2), invalid_ref(PinName::kPA0, 0);
  AnalogIn nan(PinName::kPA0, std::numeric_limits<float>::quiet_NaN());
  assert(!invalid.Initialize() && !invalid_ref.Initialize() &&
         !nan.Initialize());
}
