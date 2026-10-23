#if defined(STM32F446xx)

#include "f4/time.h"

#include "stm32f4xx.h"  // IWYU pragma: keep
#include "stm32f4xx_hal.h"

namespace nkrc::nbed::stm32::f4 {
namespace {
struct TimeState {
  uint32_t cycles_per_ms{1};
  volatile uint32_t dwt_cycles_at_last_tick{0};
  bool dwt_enabled{false};
  volatile uint32_t last_tick{0};
  volatile uint32_t tick_wraps{0};
};

TimeState& State() {
  static TimeState state{};
  return state;
}
}  // namespace

bool Time::Initialize() {
  TimeState& state = State();
  state.last_tick = HAL_GetTick();
  state.tick_wraps = 0;
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
  const uint32_t tick = HAL_GetTick();
  if (tick < state.last_tick) {
    state.tick_wraps = state.tick_wraps + 1U;
  }
  state.last_tick = tick;
  if (state.dwt_enabled) {
    state.dwt_cycles_at_last_tick = DWT->CYCCNT;
  }
}

uint32_t Time::Milliseconds() { return HAL_GetTick(); }

uint64_t Time::Microseconds() {
  const TimeState& state = State();
  uint32_t first_tick = 0;
  uint32_t second_tick = 0;
  uint32_t wraps = 0;
  uint32_t dwt_cycles_at_tick = 0;
  uint32_t cycles_now = 0;
  // Sample CYCCNT before the final tick check: a SysTick between the check
  // and CYCCNT would otherwise pair a new cycle baseline with the old tick.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
  do {
    first_tick = Milliseconds();
    wraps = state.tick_wraps;
    dwt_cycles_at_tick = state.dwt_cycles_at_last_tick;
    cycles_now = DWT->CYCCNT;
    second_tick = Milliseconds();
  } while (first_tick != second_tick);

  const uint64_t milliseconds =
      (static_cast<uint64_t>(wraps) << 32) | second_tick;
  if (!state.dwt_enabled) {
    return milliseconds * 1'000U;
  }
  const uint32_t elapsed_cycles = cycles_now - dwt_cycles_at_tick;
  const uint64_t sub_milliseconds =
      (static_cast<uint64_t>(elapsed_cycles) * 1'000U) / state.cycles_per_ms;
  return milliseconds * 1'000U +
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

bool Timer::IsRunning() const { return running_; }

uint64_t Timer::ElapsedMicroseconds() const {
  return elapsed_us_ + (running_ ? Time::Microseconds() - started_at_us_ : 0U);
}

}  // namespace nkrc::nbed::stm32::f4

#endif
