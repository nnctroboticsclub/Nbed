#if defined(STM32F767xx)

#include "f7/spi.h"

#include <initializer_list>

#include "detail/spi_master.h"
#include "f7/pin_mapper.h"
#include "stm32f7xx_hal_gpio_ex.h"
#include "stm32f7xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f7 {
namespace {
Spi*& InstanceFor(SpiPeripheral peripheral) {
  static Spi* instances[3]{};
  // Called only after IsValidPinSet() has validated the peripheral.
  return instances[static_cast<unsigned>(peripheral)];
}

void EnableSpiClock(SpiPeripheral peripheral) {
  switch (peripheral) {
    case SpiPeripheral::kSpi1:
      __HAL_RCC_SPI1_CLK_ENABLE();
      break;
    case SpiPeripheral::kSpi2:
      __HAL_RCC_SPI2_CLK_ENABLE();
      break;
    case SpiPeripheral::kSpi3:
      __HAL_RCC_SPI3_CLK_ENABLE();
      break;
  }
}

// DS11532: unlike F446, PC1 uses AF5 for SPI2, not AF7.
uint32_t AlternateFor(SpiPeripheral peripheral, interface::PinName pin) {
  using interface::PinName;
  if (peripheral == SpiPeripheral::kSpi3 && pin == PinName::kPB2) {
    return GPIO_AF7_SPI3;
  }
  if (peripheral == SpiPeripheral::kSpi3 && pin == PinName::kPD6) {
    return GPIO_AF5_SPI3;
  }
  return peripheral == SpiPeripheral::kSpi3 ? GPIO_AF6_SPI3 : GPIO_AF5_SPI1;
}
}  // namespace

Spi::Spi(SpiPeripheral peripheral, interface::PinName sck,
         interface::PinName miso, interface::PinName mosi,
         uint32_t frequency_hz, interface::SpiMode mode,
         interface::SpiBitOrder bit_order)
    : peripheral_(peripheral),
      sck_(sck),
      miso_(miso),
      mosi_(mosi),
      frequency_hz_(frequency_hz),
      mode_(mode),
      bit_order_(bit_order) {}

Spi::~Spi() {
  if (initialized_) {
    (void)HAL_SPI_DeInit(&handle_);
    InstanceFor(peripheral_) = nullptr;
  }
}

bool Spi::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!IsValidPinSet()) {
    return false;
  }
  Spi*& instance = InstanceFor(peripheral_);
  if (instance != nullptr) {
    return false;
  }
  const bool apb2 = peripheral_ == SpiPeripheral::kSpi1;
  const auto timing = detail::ComputeSpiTiming(
      apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq(), frequency_hz_,
      apb2 ? 54'000'000 : 27'000'000);
  if (!timing || !detail::ConfigureSpiMaster(handle_, timing->prescaler, mode_,
                                             bit_order_)) {
    return false;
  }
  instance = this;
  handle_.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  handle_.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  EnableSpiClock(peripheral_);
  switch (peripheral_) {
    case SpiPeripheral::kSpi1:
      handle_.Instance = SPI1;
      break;
    case SpiPeripheral::kSpi2:
      handle_.Instance = SPI2;
      break;
    case SpiPeripheral::kSpi3:
      handle_.Instance = SPI3;
      break;
  }
  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  for (const auto pin : {sck_, miso_, mosi_}) {
    gpio.Alternate = AlternateFor(peripheral_, pin);
    if (!PinMapper::ConfigureGpio(pin, gpio)) {
      instance = nullptr;
      return false;
    }
  }
  if (HAL_SPI_Init(&handle_) != HAL_OK) {
    (void)HAL_SPI_DeInit(&handle_);
    instance = nullptr;
    return false;
  }
  // Drive the configured idle SCK level before the caller asserts CS.
  __HAL_SPI_ENABLE(&handle_);
  actual_frequency_hz_ = timing->actual_hz;
  initialized_ = true;
  return true;
}

uint32_t Spi::GetFrequencyHz() const { return actual_frequency_hz_; }

bool Spi::Send(std::span<const uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiSend(handle_, data, timeout_ms);
}

bool Spi::Receive(std::span<uint8_t> data, uint32_t timeout_ms) {
  return initialized_ && detail::SpiReceive(handle_, data, timeout_ms);
}

bool Spi::Transfer(std::span<const uint8_t> tx, std::span<uint8_t> rx,
                   uint32_t timeout_ms) {
  return initialized_ && detail::SpiTransfer(handle_, tx, rx, timeout_ms);
}

bool Spi::IsValidPinSet() const {
  using interface::PinName;
  switch (peripheral_) {
    case SpiPeripheral::kSpi1:
      return (sck_ == PinName::kPA5 || sck_ == PinName::kPB3 ||
              sck_ == PinName::kPG11) &&
             (miso_ == PinName::kPA6 || miso_ == PinName::kPB4 ||
              miso_ == PinName::kPG9) &&
             (mosi_ == PinName::kPA7 || mosi_ == PinName::kPB5 ||
              mosi_ == PinName::kPD7);
    case SpiPeripheral::kSpi2:
      return (sck_ == PinName::kPA9 || sck_ == PinName::kPA12 ||
              sck_ == PinName::kPB10 || sck_ == PinName::kPB13 ||
              sck_ == PinName::kPD3) &&
             (miso_ == PinName::kPB14 || miso_ == PinName::kPC2) &&
             (mosi_ == PinName::kPB15 || mosi_ == PinName::kPC1 ||
              mosi_ == PinName::kPC3);
    case SpiPeripheral::kSpi3:
      return (sck_ == PinName::kPB3 || sck_ == PinName::kPC10) &&
             (miso_ == PinName::kPB4 || miso_ == PinName::kPC11) &&
             (mosi_ == PinName::kPB2 || mosi_ == PinName::kPB5 ||
              mosi_ == PinName::kPC12 || mosi_ == PinName::kPD6);
  }
  return false;
}

}  // namespace nkrc::nbed::stm32::f7

#endif
