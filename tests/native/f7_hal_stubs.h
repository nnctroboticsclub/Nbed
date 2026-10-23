#pragma once

#include <array>

#include "hal_stubs.h"

inline constexpr uint32_t HSE_VALUE = 8'000'000;
inline constexpr uint32_t RCC_SYSCLK_DIV1 = 1, RCC_SYSCLK_DIV2 = 2,
                          RCC_SYSCLK_DIV4 = 4, RCC_SYSCLK_DIV8 = 8,
                          RCC_SYSCLK_DIV16 = 16, RCC_SYSCLK_DIV64 = 64,
                          RCC_SYSCLK_DIV128 = 128, RCC_SYSCLK_DIV256 = 256,
                          RCC_SYSCLK_DIV512 = 512;
inline constexpr uint32_t RCC_HCLK_DIV1 = 1, RCC_HCLK_DIV2 = 2,
                          RCC_HCLK_DIV4 = 4, RCC_HCLK_DIV8 = 8,
                          RCC_HCLK_DIV16 = 16;
inline constexpr uint32_t FLASH_LATENCY_7 = 7;
inline constexpr uint32_t PWR_REGULATOR_VOLTAGE_SCALE1 = 1;
inline constexpr uint32_t RCC_OSCILLATORTYPE_HSE = 1,
                          RCC_OSCILLATORTYPE_HSI = 2, RCC_HSE_OFF = 0,
                          RCC_HSE_BYPASS = 1, RCC_HSE_ON = 2, RCC_HSI_ON = 1,
                          RCC_HSICALIBRATION_DEFAULT = 16, RCC_PLL_ON = 1,
                          RCC_PLLSOURCE_HSE = 1, RCC_PLLSOURCE_HSI = 2,
                          RCC_PLLP_DIV2 = 2, RCC_PLLP_DIV4 = 4,
                          RCC_PLLP_DIV6 = 6, RCC_PLLP_DIV8 = 8,
                          RCC_CLOCKTYPE_HCLK = 1, RCC_CLOCKTYPE_SYSCLK = 2,
                          RCC_CLOCKTYPE_PCLK1 = 4, RCC_CLOCKTYPE_PCLK2 = 8,
                          RCC_SYSCLKSOURCE_PLLCLK = 2;
#define IS_RCC_PLLM_VALUE(m) ((m) >= 2 && (m) <= 63)
#define IS_RCC_PLLP_VALUE(p) ((p) == 2 || (p) == 4 || (p) == 6 || (p) == 8)
#define IS_RCC_PLLQ_VALUE(q) ((q) >= 2 && (q) <= 15)
#define __HAL_RCC_PWR_CLK_ENABLE() ((void)0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(scale) ((void)(scale))
inline constexpr uint32_t RCC_TIMPRES_DESACTIVATED = 0;
inline uint32_t timer_prescaler = 1;
#define __HAL_RCC_TIMCLKPRESCALER(value) (timer_prescaler = (value))

struct RCC_OscInitTypeDef {
  uint32_t OscillatorType{}, HSEState{}, HSIState{}, HSICalibrationValue{};

  struct {
    uint32_t PLLState{}, PLLSource{}, PLLM{}, PLLN{}, PLLP{}, PLLQ{}, PLLR{};
  } PLL;
};

struct RCC_ClkInitTypeDef {
  uint32_t ClockType{}, SYSCLKSource{}, AHBCLKDivider{}, APB1CLKDivider{},
      APB2CLKDivider{};
};

inline RCC_OscInitTypeDef last_osc;
inline RCC_ClkInitTypeDef last_clock;
inline unsigned osc_calls = 0, overdrive_calls = 0;
inline uint32_t last_flash_latency = 0, sysclk_hz = 16'000'000;
inline HAL_StatusTypeDef osc_status = HAL_OK, overdrive_status = HAL_OK,
                         clock_status = HAL_OK;
inline std::vector<uint32_t> delay_calls;

inline HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef* config) {
  ++osc_calls;
  last_osc = *config;
  return osc_status;
}

inline HAL_StatusTypeDef HAL_PWREx_EnableOverDrive() {
  ++overdrive_calls;
  return overdrive_status;
}

inline HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef* config,
                                             uint32_t latency) {
  last_clock = *config;
  last_flash_latency = latency;
  if (clock_status == HAL_OK) {
    const uint32_t source =
        last_osc.PLL.PLLSource == RCC_PLLSOURCE_HSE ? HSE_VALUE : 16'000'000;
    sysclk_hz = uint64_t{source} * last_osc.PLL.PLLN / last_osc.PLL.PLLM /
                last_osc.PLL.PLLP;
    SystemCoreClock = sysclk_hz / config->AHBCLKDivider;
    pclk1_hz = SystemCoreClock / config->APB1CLKDivider;
    pclk2_hz = SystemCoreClock / config->APB2CLKDivider;
  }
  return clock_status;
}

inline uint32_t HAL_RCC_GetSysClockFreq() { return sysclk_hz; }

inline uint32_t HAL_RCC_GetHCLKFreq() { return SystemCoreClock; }

inline void HAL_Delay(uint32_t ms) { delay_calls.push_back(ms); }

inline void HAL_IncTick() { ++hal_tick; }

inline constexpr uint32_t GPIO_MODE_ANALOG = 20, GPIO_MODE_AF_OD = 21,
                          GPIO_AF4_I2C1 = 4;
inline constexpr uint32_t DISABLE = 0, ADC_CLOCK_SYNC_PCLK_DIV4 = 4,
                          ADC_RESOLUTION_12B = 12,
                          ADC_EXTERNALTRIGCONVEDGE_NONE = 0,
                          ADC_SOFTWARE_START = 0, ADC_DATAALIGN_RIGHT = 0,
                          ADC_EOC_SINGLE_CONV = 1, ADC_SAMPLETIME_15CYCLES = 15;
inline constexpr uint32_t ADC_CHANNEL_0 = 0, ADC_CHANNEL_1 = 1,
                          ADC_CHANNEL_2 = 2, ADC_CHANNEL_3 = 3,
                          ADC_CHANNEL_4 = 4, ADC_CHANNEL_5 = 5,
                          ADC_CHANNEL_6 = 6, ADC_CHANNEL_7 = 7,
                          ADC_CHANNEL_8 = 8, ADC_CHANNEL_9 = 9,
                          ADC_CHANNEL_10 = 10, ADC_CHANNEL_11 = 11,
                          ADC_CHANNEL_12 = 12, ADC_CHANNEL_13 = 13,
                          ADC_CHANNEL_14 = 14, ADC_CHANNEL_15 = 15;

struct ADC_TypeDef {};

inline ADC_TypeDef f7_adc1, f7_adc3;
#define ADC1 (&f7_adc1)
#define ADC3 (&f7_adc3)
#define __HAL_RCC_ADC1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC3_CLK_ENABLE() ((void)0)

struct ADC_InitTypeDef {
  uint32_t ClockPrescaler{}, Resolution{}, ScanConvMode{}, ContinuousConvMode{},
      DiscontinuousConvMode{}, ExternalTrigConvEdge{}, ExternalTrigConv{},
      DataAlign{}, NbrOfConversion{}, DMAContinuousRequests{}, EOCSelection{};
};

struct ADC_HandleTypeDef {
  ADC_TypeDef* Instance{};
  ADC_InitTypeDef Init{};
};

struct ADC_ChannelConfTypeDef {
  uint32_t Channel{}, Rank{}, SamplingTime{};
};

inline HAL_StatusTypeDef adc_init_status = HAL_OK, adc_config_status = HAL_OK,
                         adc_poll_status = HAL_OK;
inline ADC_HandleTypeDef last_adc_init;
inline unsigned adc_stops = 0;
inline std::vector<std::pair<ADC_TypeDef*, uint32_t>> adc_channels;

inline HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* handle) {
  last_adc_init = *handle;
  return adc_init_status;
}

inline HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* handle,
                                               ADC_ChannelConfTypeDef* config) {
  adc_channels.emplace_back(handle->Instance, config->Channel);
  return adc_config_status;
}

inline HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*,
                                                   uint32_t) {
  return adc_poll_status;
}

inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*) { return 2048; }

inline HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*) {
  ++adc_stops;
  return HAL_OK;
}

struct I2C_TypeDef {};

inline I2C_TypeDef i2c1, i2c2, i2c3;
#define I2C1 (&i2c1)
#define I2C2 (&i2c2)
#define I2C3 (&i2c3)
inline constexpr uint32_t RCC_I2C1CLKSOURCE_HSI = 1, RCC_I2C2CLKSOURCE_HSI = 1,
                          RCC_I2C3CLKSOURCE_HSI = 1;
inline std::array<uint32_t, 3> i2c_clocks{};
#define __HAL_RCC_I2C1_CONFIG(source) (i2c_clocks[0] = (source))
#define __HAL_RCC_I2C2_CONFIG(source) (i2c_clocks[1] = (source))
#define __HAL_RCC_I2C3_CONFIG(source) (i2c_clocks[2] = (source))
#define __HAL_RCC_I2C1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_I2C3_CLK_ENABLE() ((void)0)
inline constexpr uint32_t I2C_OA2_NOMASK = 0, I2C_ADDRESSINGMODE_7BIT = 7,
                          I2C_DUALADDRESS_DISABLE = 0,
                          I2C_GENERALCALL_DISABLE = 0,
                          I2C_NOSTRETCH_DISABLE = 0,
                          I2C_ANALOGFILTER_ENABLE = 1;

struct I2C_InitTypeDef {
  uint32_t Timing{}, OwnAddress2Masks{}, OwnAddress1{}, AddressingMode{},
      DualAddressMode{}, OwnAddress2{}, GeneralCallMode{}, NoStretchMode{};
};

struct I2C_HandleTypeDef {
  I2C_TypeDef* Instance{};
  I2C_InitTypeDef Init{};
};

inline I2C_HandleTypeDef last_i2c_init;
inline HAL_StatusTypeDef i2c_init_status = HAL_OK, i2c_filter_status = HAL_OK,
                         i2c_transfer_status = HAL_OK;
inline unsigned i2c_deinits = 0, i2c_transfers = 0;
inline uint16_t last_i2c_address = 0;

inline HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef* handle) {
  last_i2c_init = *handle;
  return i2c_init_status;
}

inline HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef*) {
  ++i2c_deinits;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_I2CEx_ConfigAnalogFilter(I2C_HandleTypeDef*,
                                                      uint32_t) {
  return i2c_filter_status;
}

inline HAL_StatusTypeDef HAL_I2CEx_ConfigDigitalFilter(I2C_HandleTypeDef*,
                                                       uint32_t) {
  return i2c_filter_status;
}

inline HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef*,
                                                 uint16_t address, uint8_t*,
                                                 uint16_t, uint32_t) {
  ++i2c_transfers;
  last_i2c_address = address;
  return i2c_transfer_status;
}

inline HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* handle,
                                                uint16_t address, uint8_t* data,
                                                uint16_t size,
                                                uint32_t timeout) {
  return HAL_I2C_Master_Transmit(handle, address, data, size, timeout);
}
