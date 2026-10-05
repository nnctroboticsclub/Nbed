#pragma once

#include <cstdint>

#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_uart.h"

#include "interface/pin_names.h"
#include "interface/uart.h"

namespace nbed::f4 {

enum class UartPeripheral : uint8_t { kUsart2, kUsart3, kUart4 };

class Uart final : public interface::Uart {
 public:
  Uart(UartPeripheral peripheral, interface::PinName rx, interface::PinName tx,
       uint32_t baud_rate = 921600, uint32_t interrupt_priority = 0);
  ~Uart() override;
  Uart(const Uart&) = delete;
  Uart& operator=(const Uart&) = delete;
  Uart(Uart&&) = delete;
  Uart& operator=(Uart&&) = delete;

  bool Initialize() override;

  bool Send(interface::UartTxMessage message) override;
  bool Receive(interface::UartRxMessage message) override;

  bool EnableRxInterrupt() override;
  void SetRxCallback(const interface::UartRxCallback& callback) override;
  void SetConsole() override;

  [[nodiscard]] UART_HandleTypeDef& GetHandle();

  static void HandleInterrupt(UartPeripheral peripheral);
  static void HandleRxComplete(UART_HandleTypeDef* handle);

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  [[nodiscard]] IRQn_Type Irq() const;
  static Uart*& InstanceFor(UartPeripheral peripheral);
  void OnRxComplete();

  UartPeripheral peripheral_;
  interface::PinName rx_;
  interface::PinName tx_;
  uint32_t baud_rate_;
  uint32_t interrupt_priority_;
  UART_HandleTypeDef handle_{};
  interface::UartRxCallback rx_callback_;
  uint8_t rx_byte_{0};
  bool initialized_{false};
};

}  // namespace nbed::f4
