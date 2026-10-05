#pragma once

#include <cstdint>

#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_tim.h"

#include "interface/encoder.h"
#include "interface/pin_names.h"

namespace nbed::f4 {

enum class EncoderTimer : uint8_t { kTim1, kTim2, kTim3 };

class Encoder final : public interface::Encoder {
 public:
  Encoder(EncoderTimer timer, interface::PinName channel_a,
          interface::PinName channel_b, uint32_t counts_per_revolution);
  ~Encoder() override;

  Encoder(const Encoder&) = delete;
  Encoder& operator=(const Encoder&) = delete;
  Encoder(Encoder&&) = delete;
  Encoder& operator=(Encoder&&) = delete;

  bool Initialize() override;

  [[nodiscard]] int GetCount() override;
  [[nodiscard]] Angle GetAngle() override;

  void ResetCount() override;

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  EncoderTimer timer_;
  interface::PinName channel_a_;
  interface::PinName channel_b_;
  uint32_t counts_per_revolution_;
  TIM_HandleTypeDef handle_{};
  bool initialized_{false};
};

}  // namespace nbed::f4
