#pragma once

#include <cstdint>
#include <vector>

struct GPIO_TypeDef {
  uint32_t IDR{}, ODR{};
};

struct GPIO_InitTypeDef {
  uint32_t Pin{}, Mode{}, Pull{}, Speed{}, Alternate{};
};

inline GPIO_TypeDef gpio_a, gpio_b, gpio_c, gpio_d, gpio_f, gpio_h;
#define GPIOA (&gpio_a)
#define GPIOB (&gpio_b)
#define GPIOC (&gpio_c)
#define GPIOD (&gpio_d)
#define GPIOF (&gpio_f)
#define GPIOH (&gpio_h)
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOD_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOF_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOH_CLK_ENABLE() ((void)0)

enum GPIO_PinState { GPIO_PIN_RESET, GPIO_PIN_SET };

enum {
  GPIO_NOPULL,
  GPIO_PULLUP,
  GPIO_PULLDOWN,
  GPIO_MODE_INPUT,
  GPIO_MODE_OUTPUT_PP,
  GPIO_MODE_AF_PP,
  GPIO_SPEED_FREQ_LOW,
  GPIO_SPEED_FREQ_HIGH,
  GPIO_SPEED_FREQ_VERY_HIGH,
  GPIO_AF7_USART2,
  GPIO_AF8_UART4
};

inline void HAL_GPIO_Init(GPIO_TypeDef*, GPIO_InitTypeDef*) {}

inline void HAL_GPIO_WritePin(GPIO_TypeDef* port, uint16_t pin,
                              GPIO_PinState state) {
  port->ODR = state == GPIO_PIN_SET ? port->ODR | pin : port->ODR & ~pin;
}

inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* port, uint16_t pin) {
  return port->IDR & pin ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

struct USART_TypeDef {};

inline USART_TypeDef usart1, usart2, usart3, uart4;
#define USART1 (&usart1)
#define USART2 (&usart2)
#define USART3 (&usart3)
#define UART4 (&uart4)
#define __HAL_RCC_USART1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART3_CLK_ENABLE() ((void)0)
#define __HAL_RCC_UART4_CLK_ENABLE() ((void)0)

enum IRQn_Type { USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn };

struct UART_InitTypeDef {
  uint32_t BaudRate{}, WordLength{}, StopBits{}, Parity{}, Mode{}, HwFlowCtl{},
      OverSampling{};
};

struct UART_HandleTypeDef {
  USART_TypeDef* Instance{};
  UART_InitTypeDef Init{};
};

enum {
  UART_WORDLENGTH_8B,
  UART_STOPBITS_1,
  UART_PARITY_NONE,
  UART_MODE_TX_RX,
  UART_HWCONTROL_NONE,
  UART_OVERSAMPLING_16
};

enum HAL_StatusTypeDef { HAL_OK, HAL_ERROR };

inline constexpr uint32_t HAL_MAX_DELAY = 0xffffffffU;
inline std::vector<uint16_t> tx_sizes;
inline std::vector<uint8_t> tx_data;
inline int fail_tx_call = -1;

inline HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef*) { return HAL_OK; }

inline HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t* data,
                                           uint16_t size, uint32_t) {
  tx_sizes.push_back(size);
  if (static_cast<int>(tx_sizes.size()) == fail_tx_call) {
    return HAL_ERROR;
  }
  tx_data.insert(tx_data.end(), data, data + size);
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef*, uint8_t*,
                                          uint16_t, uint32_t) {
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*, uint8_t*,
                                             uint16_t) {
  return HAL_OK;
}

inline void HAL_UART_IRQHandler(UART_HandleTypeDef*) {}

inline void HAL_NVIC_SetPriority(IRQn_Type, uint32_t, uint32_t) {}

inline void HAL_NVIC_EnableIRQ(IRQn_Type) {}

inline void HAL_NVIC_DisableIRQ(IRQn_Type) {}

struct CoreDebug_Type {
  uint32_t DEMCR{};
};

struct DWT_Type {
  uint32_t CTRL{}, CYCCNT{};
};

inline CoreDebug_Type core_debug;
inline DWT_Type dwt;
#define CoreDebug (&core_debug)
#define DWT (&dwt)
inline constexpr uint32_t CoreDebug_DEMCR_TRCENA_Msk = 1U;
inline constexpr uint32_t DWT_CTRL_CYCCNTENA_Msk = 1U;
inline uint32_t SystemCoreClock = 64'000'000;
inline uint32_t hal_tick = 0;
inline void (*tick_hook)() = nullptr;

inline uint32_t HAL_GetTick() {
  if (tick_hook != nullptr) {
    auto hook = tick_hook;
    tick_hook = nullptr;
    hook();
  }
  return hal_tick;
}
