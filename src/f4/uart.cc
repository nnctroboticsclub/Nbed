#if defined(STM32F446xx)

#include "f4/uart.h"

#include <limits>

#include "stm32f4xx_hal.h"  // IWYU pragma: keep
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

#include "f4/pin_mapper.h"

namespace nbed::f4 {
namespace {
Uart*& Usart2Instance() {
  static Uart* instance = nullptr;
  return instance;
}

Uart*& Usart3Instance() {
  static Uart* instance = nullptr;
  return instance;
}

Uart*& Uart4Instance() {
  static Uart* instance = nullptr;
  return instance;
}

Uart*& Console() {
  static Uart* instance = nullptr;
  return instance;
}

void EnableUartClock(UartPeripheral peripheral) {
  if (peripheral == UartPeripheral::kUsart2) {
    __HAL_RCC_USART2_CLK_ENABLE();  // NOLINT
  } else if (peripheral == UartPeripheral::kUsart3) {
    __HAL_RCC_USART3_CLK_ENABLE();  // NOLINT
  } else {
    __HAL_RCC_UART4_CLK_ENABLE();  // NOLINT
  }
}
}  // namespace

Uart::Uart(UartPeripheral peripheral, interface::PinName rx,
           interface::PinName tx, uint32_t baud_rate,
           uint32_t interrupt_priority)
    : peripheral_(peripheral),
      rx_(rx),
      tx_(tx),
      baud_rate_(baud_rate),
      interrupt_priority_(interrupt_priority) {
}

Uart::~Uart() {
  if (!initialized_) {
    return;
  }
  HAL_NVIC_DisableIRQ(Irq());
  (void)HAL_UART_DeInit(&handle_);
  if (InstanceFor(peripheral_) == this) {
    InstanceFor(peripheral_) = nullptr;
  }
  if (Console() == this) {
    Console() = nullptr;
  }
  initialized_ = false;
}

bool Uart::Initialize() {
  if (initialized_) {
    return true;
  }
  Uart*& instance = InstanceFor(peripheral_);
  if (!IsValidPinPair() || baud_rate_ == 0 ||
      (instance != nullptr && instance != this)) {
    return false;
  }
  instance = this;
  EnableUartClock(peripheral_);
  if (peripheral_ == UartPeripheral::kUsart2) {         // NOLINT
    handle_.Instance = USART2;                          // NOLINT
  } else if (peripheral_ == UartPeripheral::kUsart3) {  // NOLINT
    handle_.Instance = USART3;                          // NOLINT
  } else {
    handle_.Instance = UART4;  // NOLINT
  }
  handle_.Init.BaudRate = baud_rate_;
  handle_.Init.WordLength = UART_WORDLENGTH_8B;
  handle_.Init.StopBits = UART_STOPBITS_1;
  handle_.Init.Parity = UART_PARITY_NONE;
  handle_.Init.Mode = UART_MODE_TX_RX;
  handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  handle_.Init.OverSampling = UART_OVERSAMPLING_16;
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate =
      peripheral_ == UartPeripheral::kUart4 ? GPIO_AF8_UART4 : GPIO_AF7_USART2;
  if (!PinMapper::ConfigureGpio(rx_, gpio) ||
      !PinMapper::ConfigureGpio(tx_, gpio)) {
    instance = nullptr;
    return false;
  }
  if (HAL_UART_Init(&handle_) != HAL_OK) {
    instance = nullptr;
    return false;
  }
  HAL_NVIC_SetPriority(Irq(), interrupt_priority_, 0);
  HAL_NVIC_EnableIRQ(Irq());
  initialized_ = true;
  return true;
}

bool Uart::Send(interface::UartTxMessage message) {
  return initialized_ &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Transmit(&handle_,
                           const_cast<uint8_t*>(message.data()),  // NOLINT
                           static_cast<uint16_t>(message.size()),
                           HAL_MAX_DELAY) == HAL_OK;
}

bool Uart::Receive(interface::UartRxMessage message) {
  return initialized_ &&
         message.size() <= std::numeric_limits<uint16_t>::max() &&
         HAL_UART_Receive(&handle_, message.data(),
                          static_cast<uint16_t>(message.size()),
                          HAL_MAX_DELAY) == HAL_OK;
}

bool Uart::EnableRxInterrupt() {
  return initialized_ && HAL_UART_Receive_IT(&handle_, &rx_byte_, 1) == HAL_OK;
}

void Uart::SetRxCallback(const interface::UartRxCallback& callback) {
  rx_callback_ = callback;
}

UART_HandleTypeDef& Uart::GetHandle() {
  return handle_;
}

void Uart::SetConsole() {
  Console() = this;
}

void Uart::HandleInterrupt(UartPeripheral peripheral) {
  Uart* const uart = InstanceFor(peripheral);
  if (uart != nullptr && uart->initialized_) {
    HAL_UART_IRQHandler(&uart->handle_);
  }
}

void Uart::HandleRxComplete(UART_HandleTypeDef* handle) {
  if (handle == nullptr) {
    return;
  }
  Uart* uart = nullptr;
  if (handle->Instance == USART2) {  // NOLINT
    uart = Usart2Instance();
  } else if (handle->Instance == USART3) {  // NOLINT
    uart = Usart3Instance();
  } else if (handle->Instance == UART4) {  // NOLINT
    uart = Uart4Instance();
  }
  if (uart != nullptr) {
    uart->OnRxComplete();
  }
}

void Uart::OnRxComplete() {
  if (rx_callback_) {
    rx_callback_(interface::UartRxMessage{&rx_byte_, 1});
  }
  (void)EnableRxInterrupt();
}

bool Uart::IsValidPinPair() const {
  using interface::PinName;
  if (!PinMapper::IsValidPinPair(rx_, tx_)) {
    return false;
  }
  if (peripheral_ == UartPeripheral::kUsart2) {
    return rx_ == PinName::kPA3 && tx_ == PinName::kPA2;
  }
  if (peripheral_ == UartPeripheral::kUsart3) {
    return (rx_ == PinName::kPC5 || rx_ == PinName::kPC11) &&
           (tx_ == PinName::kPB10 || tx_ == PinName::kPC10);
  }
  return (rx_ == PinName::kPA1 || rx_ == PinName::kPC11) &&
         (tx_ == PinName::kPA0 || tx_ == PinName::kPC10);
}

IRQn_Type Uart::Irq() const {
  if (peripheral_ == UartPeripheral::kUsart2) {
    return USART2_IRQn;
  }
  if (peripheral_ == UartPeripheral::kUsart3) {
    return USART3_IRQn;
  }
  return UART4_IRQn;
}

Uart*& Uart::InstanceFor(UartPeripheral peripheral) {
  if (peripheral == UartPeripheral::kUsart2) {
    return Usart2Instance();
  }
  if (peripheral == UartPeripheral::kUsart3) {
    return Usart3Instance();
  }
  return Uart4Instance();
}

extern "C" {
void USART2_IRQHandler() {  // NOLINT
  Uart::HandleInterrupt(UartPeripheral::kUsart2);
}

void USART3_IRQHandler() {  // NOLINT
  Uart::HandleInterrupt(UartPeripheral::kUsart3);
}

void UART4_IRQHandler() {  // NOLINT
  Uart::HandleInterrupt(UartPeripheral::kUart4);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {  // NOLINT
  Uart::HandleRxComplete(huart);
}

int __io_putchar(int ch) {  // NOLINT
  if (Console() == nullptr) {
    return ch;
  }
  const uint8_t byte = static_cast<uint8_t>(ch);
  (void)Console()->Send(interface::UartTxMessage{&byte, 1});
  return ch;
}

int __io_getchar() {  // NOLINT
  if (Console() == nullptr) {
    return -1;
  }
  uint8_t byte = 0;
  return Console()->Receive(interface::UartRxMessage{&byte, 1})
             ? static_cast<int>(byte)
             : -1;
}
}  // extern "C"

}  // namespace nbed::f4

#endif
