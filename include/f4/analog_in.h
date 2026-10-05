#pragma once
// IWYU pragma: private, include "nbed.h"

#include <cstdint>

#include "stm32f4xx_hal.h"  // IWYU pragma: export

#include "interface/analog_in.h"
#include "interface/pin_names.h"

namespace nbed::f4 {

class AnalogIn final : public interface::AnalogIn {
 public:
  explicit AnalogIn(interface::PinName pin, float reference_voltage = 3.3F);
  ~AnalogIn() override = default;
  AnalogIn(const AnalogIn&) = delete;
  AnalogIn& operator=(const AnalogIn&) = delete;
  AnalogIn(AnalogIn&&) = delete;
  AnalogIn& operator=(AnalogIn&&) = delete;

  bool Initialize() override;

  [[nodiscard]] float ReadRatio() override;
  [[nodiscard]] float ReadVoltage() override;

 private:
  [[nodiscard]] bool IsValidPin() const;
  [[nodiscard]] uint32_t Channel() const;
  interface::PinName pin_;
  float reference_voltage_;
  bool initialized_{false};
};

}  // namespace nbed::f4
