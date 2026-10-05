#pragma once

#include <cstdint>

#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_can.h"

#include "interface/can.h"
#include "interface/pin_names.h"

namespace nbed::f4 {

enum class CanPeripheral : uint8_t { kCan1, kCan2 };
enum class CanMode : uint8_t { kNormal, kLoopback, kSilent, kSilentLoopback };

struct CanConfig {
  CanPeripheral peripheral{CanPeripheral::kCan1};
  interface::PinName rx{interface::PinName::kNC};
  interface::PinName tx{interface::PinName::kNC};
  uint32_t bitrate{1'000'000};
  CanMode mode{CanMode::kNormal};
  uint32_t interrupt_priority{0};
};

struct CanFilter {
  uint32_t id{0};
  uint32_t mask{0};
  interface::CanFormat format{interface::CanFormat::kStandard};
  uint8_t bank{0xff};
  uint32_t fifo{CAN_RX_FIFO0};
};

class Can final : public interface::Can {
 public:
  explicit Can(CanConfig config);
  Can(CanPeripheral peripheral, interface::PinName rx, interface::PinName tx,
      uint32_t bitrate = 1'000'000);
  ~Can() override;

  Can(const Can&) = delete;
  Can& operator=(const Can&) = delete;
  Can(Can&&) = delete;
  Can& operator=(Can&&) = delete;

  // --- Lifecycle ---
  bool Initialize() override;
  bool Start() override;
  void Stop() override;

  // --- Transmit / Receive ---
  bool Send(const interface::CanMessage& message) override;
  bool Receive(interface::CanMessage& message) override;

  // --- Rx filter ---
  bool SetRxFilter(const CanFilter& filter);
  bool SetRxFilter(const interface::CanRxFilter& filter) override;
  bool SetRxFilter(uint32_t id, uint32_t mask) override;

  // --- Rx interrupt ---
  bool EnableRxInterrupt() override;
  void SetRxCallback(const interface::CanRxCallback& callback) override;

  // --- Status ---
  [[nodiscard]] uint8_t GetSendErrorCount() const override;
  [[nodiscard]] uint8_t GetReceiveErrorCount() const override;
  [[nodiscard]] CAN_HandleTypeDef& GetHandle();

  // --- HAL glue (called from ISR / MSP callbacks) ---
  static void HandleInterrupt(CanPeripheral peripheral);
  static void HandleRxPending(CAN_HandleTypeDef* handle);
  static void ConfigureMsp(CAN_HandleTypeDef* handle);
  static void DeconfigureMsp(CAN_HandleTypeDef* handle);

 private:
  [[nodiscard]] bool IsValidPinPair() const;
  [[nodiscard]] bool ConfigureBitTiming();
  [[nodiscard]] bool OwnPeripheral();
  void ReleasePeripheral();
  void OnRxPending();
  [[nodiscard]] uint8_t DefaultFilterBank() const;
  [[nodiscard]] static Can*& InstanceFor(CanPeripheral peripheral);
  [[nodiscard]] static Can* FromHandle(CAN_HandleTypeDef* handle);

  CanConfig config_;
  CAN_HandleTypeDef handle_{};
  interface::CanRxCallback rx_callback_;
  bool initialized_{false};
  bool started_{false};
};

}  // namespace nbed::f4
