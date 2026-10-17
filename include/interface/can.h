#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace nbed::interface {

enum class CanFormat : uint8_t { kStandard, kExtended };
enum class CanFrameType : uint8_t { kData, kRemote };

struct CanMessage {
  uint32_t id{0};
  uint8_t size{0};
  std::array<uint8_t, 8> data{};
  CanFormat format{CanFormat::kStandard};
  CanFrameType type{CanFrameType::kData};
};

using CanRxCallback = std::function<void(const CanMessage&)>;

struct CanRxFilter {
  uint32_t id{0};
  uint32_t mask{0};
  CanFormat format{CanFormat::kStandard};
};

class Can {
 public:
  Can() = default;
  virtual ~Can() = default;
  Can(const Can&) = delete;
  Can& operator=(const Can&) = delete;
  Can(Can&&) = delete;
  Can& operator=(Can&&) = delete;

  virtual bool Initialize() = 0;
  virtual bool Start() = 0;
  virtual void Stop() = 0;

  virtual bool Send(const CanMessage& message) = 0;
  virtual bool Receive(CanMessage& message) = 0;

  virtual bool SetRxFilter(const CanRxFilter& filter) = 0;
  virtual bool SetRxFilter(uint32_t id, uint32_t mask) = 0;

  virtual bool EnableRxInterrupt() = 0;
  virtual void SetRxCallback(const CanRxCallback& callback) = 0;

  [[nodiscard]] virtual uint8_t GetSendErrorCount() const = 0;
  [[nodiscard]] virtual uint8_t GetReceiveErrorCount() const = 0;
};

}  // namespace nbed::interface
