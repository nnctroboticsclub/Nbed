#if defined(STM32F446xx)

#include "f4/time.h"

#include "stm32f4xx.h"  // IWYU pragma: keep
#include "stm32f4xx_hal.h"

namespace nbed::f4 {
namespace {
struct TimeState {
  uint32_t cycles_per_ms{1};
  volatile uint32_t dwt_cycles_at_last_tick{0};
  bool dwt_enabled{false};
};

TimeState& State() {
  static TimeState state{};
  return state;
}
}  // namespace

bool Time::Initialize() {
  TimeState& state = State();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  state.cycles_per_ms = SystemCoreClock / 1'000U;
  if (state.cycles_per_ms == 0U) {
    state.cycles_per_ms = 1U;
  }
  state.dwt_enabled = (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U;
  state.dwt_cycles_at_last_tick = DWT->CYCCNT;
  return state.dwt_enabled;
}

void Time::OnSysTick() {
  TimeState& state = State();
  if (state.dwt_enabled) {
    state.dwt_cycles_at_last_tick = DWT->CYCCNT;
  }
}

uint32_t Time::Milliseconds() {
  return HAL_GetTick();
}

uint64_t Time::Microseconds() {
  const TimeState& state = State();
  const uint64_t milliseconds = Milliseconds();
  if (!state.dwt_enabled) {
    return milliseconds * 1'000U;
  }

  // Align the DWT counter to the SysTick that maintains HAL_GetTick(). Taking
  // the old implementation's modulo against the free-running DWT counter can
  // otherwise make the reported time move backwards at a millisecond boundary.
  uint32_t first_tick = 0;
  uint32_t second_tick = 0;
  uint32_t dwt_cycles_at_tick = 0;
  do {  // NOLINT
    first_tick = Milliseconds();
    dwt_cycles_at_tick = state.dwt_cycles_at_last_tick;
    second_tick = Milliseconds();
  } while (first_tick != second_tick);

  const uint32_t elapsed_cycles = DWT->CYCCNT - dwt_cycles_at_tick;
  const uint64_t sub_milliseconds =
      (static_cast<uint64_t>(elapsed_cycles) * 1'000U) / state.cycles_per_ms;
  return (static_cast<uint64_t>(second_tick) * 1'000U) +
         (sub_milliseconds < 1'000U ? sub_milliseconds : 999U);
}

void Timer::Start() {
  if (!running_) {
    started_at_us_ = Time::Microseconds();
    running_ = true;
  }
}

void Timer::Stop() {
  if (running_) {
    elapsed_us_ += Time::Microseconds() - started_at_us_;
    running_ = false;
  }
}

void Timer::Reset() {
  elapsed_us_ = 0;
  if (running_) {
    started_at_us_ = Time::Microseconds();
  }
}

bool Timer::IsRunning() const {
  return running_;
}

uint64_t Timer::ElapsedMicroseconds() const {
  return elapsed_us_ + (running_ ? Time::Microseconds() - started_at_us_ : 0U);
}

}  // namespace nbed::f4

#endif
