#pragma once

#include <array>

#include "hal_stubs.h"

struct TIM_TypeDef {
  uint32_t PSC{}, ARR{};
  std::array<uint32_t, 4> CCR{};
  uint32_t active_channels{};
  bool clock_enabled{false};
};

inline TIM_TypeDef tim1, tim2, tim3, tim15, tim16, tim17;
#define TIM1 (&tim1)
#define TIM2 (&tim2)
#define TIM3 (&tim3)
#define TIM15 (&tim15)
#define TIM16 (&tim16)
#define TIM17 (&tim17)
#define __HAL_RCC_TIM1_CLK_ENABLE() (tim1.clock_enabled = true)
#define __HAL_RCC_TIM2_CLK_ENABLE() (tim2.clock_enabled = true)
#define __HAL_RCC_TIM3_CLK_ENABLE() (tim3.clock_enabled = true)
#define __HAL_RCC_TIM15_CLK_ENABLE() (tim15.clock_enabled = true)
#define __HAL_RCC_TIM16_CLK_ENABLE() (tim16.clock_enabled = true)
#define __HAL_RCC_TIM17_CLK_ENABLE() (tim17.clock_enabled = true)

inline constexpr uint32_t GPIO_AF1_TIM2 = 1, GPIO_AF2_TIM3 = 2,
                          GPIO_AF6_TIM1 = 6, GPIO_AF10_TIM2 = 10,
                          GPIO_AF10_TIM3 = 10, GPIO_AF11_TIM1 = 11,
                          GPIO_AF9_TIM15 = 9, GPIO_AF1_TIM16 = 1,
                          GPIO_AF1_TIM17 = 1, GPIO_AF10_TIM17 = 10;
inline constexpr uint32_t TIM_CHANNEL_1 = 0, TIM_CHANNEL_2 = 4,
                          TIM_CHANNEL_3 = 8, TIM_CHANNEL_4 = 12;
inline constexpr uint32_t TIM_COUNTERMODE_UP = 0, TIM_CLOCKDIVISION_DIV1 = 0,
                          TIM_AUTORELOAD_PRELOAD_ENABLE = 1,
                          TIM_OCMODE_PWM1 = 6, TIM_OCPOLARITY_HIGH = 0,
                          TIM_OCFAST_DISABLE = 0, TIM_EVENTSOURCE_UPDATE = 1;
inline constexpr uint32_t RCC_HCLK_DIV1 = 1, RCC_HCLK_DIV2 = 2,
                          RCC_HCLK_DIV4 = 4;

struct TIM_Base_InitTypeDef {
  uint32_t Prescaler{}, Period{}, CounterMode{}, ClockDivision{},
      RepetitionCounter{}, AutoReloadPreload{};
};

struct TIM_HandleTypeDef {
  TIM_TypeDef* Instance{};
  TIM_Base_InitTypeDef Init{};
};

struct TIM_OC_InitTypeDef {
  uint32_t OCMode{}, Pulse{}, OCPolarity{}, OCFastMode{};
};

struct RCC_ClkInitTypeDef {
  uint32_t APB1CLKDivider{RCC_HCLK_DIV2}, APB2CLKDivider{RCC_HCLK_DIV1};
};

inline RCC_ClkInitTypeDef pwm_clock_config;
inline TIM_HandleTypeDef last_pwm_init;
inline HAL_StatusTypeDef pwm_init_status = HAL_OK, pwm_config_status = HAL_OK,
                         pwm_start_status = HAL_OK;
inline unsigned pwm_init_calls = 0, pwm_deinit_calls = 0, pwm_updates = 0;

inline void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef* configuration,
                                   uint32_t* latency) {
  *configuration = pwm_clock_config;
  *latency = 0;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef* handle) {
  ++pwm_init_calls;
  last_pwm_init = *handle;
  handle->Instance->PSC = handle->Init.Prescaler;
  handle->Instance->ARR = handle->Init.Period;
  return pwm_init_status;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_DeInit(TIM_HandleTypeDef* handle) {
  ++pwm_deinit_calls;
  handle->Instance->active_channels = 0;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef* handle,
                                                   TIM_OC_InitTypeDef* config,
                                                   uint32_t channel) {
  if (pwm_config_status == HAL_OK) {
    handle->Instance->CCR.at(channel / 4) = config->Pulse;
  }
  return pwm_config_status;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef* handle,
                                           uint32_t channel) {
  if (pwm_start_status == HAL_OK) {
    handle->Instance->active_channels |= 1U << (channel / 4);
  }
  return pwm_start_status;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef* handle,
                                          uint32_t channel) {
  handle->Instance->active_channels &= ~(1U << (channel / 4));
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef*, uint32_t) {
  ++pwm_updates;
  return HAL_OK;
}

#define __HAL_TIM_SET_COMPARE(handle, channel, value) \
  ((handle)->Instance->CCR.at((channel) / 4) = (value))
#define __HAL_TIM_GET_COMPARE(handle, channel) \
  ((handle)->Instance->CCR.at((channel) / 4))
#define __HAL_TIM_SET_PRESCALER(handle, value) \
  ((handle)->Instance->PSC = (value))
#define __HAL_TIM_SET_AUTORELOAD(handle, value) \
  ((handle)->Instance->ARR = (value))
