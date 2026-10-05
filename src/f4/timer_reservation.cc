#if defined(STM32F446xx)

#include "f4/timer_reservation.h"

#include <array>

namespace nbed::f4 {
namespace {

struct TimerReservationState {
  uint8_t pwm_channel_mask{0};
  bool encoder_claimed{false};
};

TimerReservationState* StateFor(uint8_t timer) {
  static std::array<TimerReservationState, 3> states{};
  return timer < states.size() ? &states.at(timer) : nullptr;
}

uint8_t ChannelBit(uint8_t channel) {
  return channel < 4U ? static_cast<uint8_t>(1U << channel) : 0U;
}

}  // namespace

bool TimerReservation::ClaimPwm(uint8_t timer, uint8_t channel) {
  TimerReservationState* const state = StateFor(timer);
  const uint8_t channel_bit = ChannelBit(channel);
  if (state == nullptr || channel_bit == 0U || state->encoder_claimed ||
      (state->pwm_channel_mask & channel_bit) != 0U) {
    return false;
  }
  state->pwm_channel_mask |= channel_bit;
  return true;
}

void TimerReservation::ReleasePwm(uint8_t timer, uint8_t channel) {
  TimerReservationState* const state = StateFor(timer);
  if (state != nullptr) {
    state->pwm_channel_mask &= static_cast<uint8_t>(~ChannelBit(channel));
  }
}

bool TimerReservation::ClaimEncoder(uint8_t timer) {
  TimerReservationState* const state = StateFor(timer);
  if (state == nullptr || state->encoder_claimed ||
      state->pwm_channel_mask != 0U) {
    return false;
  }
  state->encoder_claimed = true;
  return true;
}

void TimerReservation::ReleaseEncoder(uint8_t timer) {
  TimerReservationState* const state = StateFor(timer);
  if (state != nullptr) {
    state->encoder_claimed = false;
  }
}

}  // namespace nbed::f4

#endif
