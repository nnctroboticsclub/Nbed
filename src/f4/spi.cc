#if defined(STM32F446xx)

#include "f4/spi.h"

#include <initializer_list>

#include "detail/spi_master.h"
#include "f4/pin_mapper.h"
#include "stm32f4xx_hal_gpio_ex.h"
#include "stm32f4xx_hal_rcc.h"

namespace nkrc::nbed::stm32::f4 {
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

// DS10693 table 11: most pins use AF5 (SPI1/2) or AF6 (SPI3),
// but SPI2 MOSI on PC1 and SPI3 MOSI on PB0/PB2 need AF7.
uint32_t AlternateFor(SpiPeripheral peripheral, interface::PinName pin) {
  using interface::PinName;
  if ((peripheral == SpiPeripheral::kSpi2 && pin == PinName::kPC1) ||
      (peripheral == SpiPeripheral::kSpi3 &&
       (pin == PinName::kPB0 || pin == PinName::kPB2))) {
    return peripheral == SpiPeripheral::kSpi2 ? GPIO_AF7_SPI2 : GPIO_AF7_SPI3;
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
      apb2 ? 45'000'000 : 22'500'000);
  if (!timing || !detail::ConfigureSpiMaster(handle_, timing->prescaler, mode_,
                                             bit_order_)) {
    return false;
  }
  instance = this;
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
      return (sck_ == PinName::kPA5 || sck_ == PinName::kPB3) &&
             (miso_ == PinName::kPA6 || miso_ == PinName::kPB4) &&
             (mosi_ == PinName::kPA7 || mosi_ == PinName::kPB5);
    case SpiPeripheral::kSpi2:
      return (sck_ == PinName::kPA9 || sck_ == PinName::kPB10 ||
              sck_ == PinName::kPB13 || sck_ == PinName::kPC7) &&
             (miso_ == PinName::kPB14 || miso_ == PinName::kPC2) &&
             (mosi_ == PinName::kPB15 || mosi_ == PinName::kPC1 ||
              mosi_ == PinName::kPC3);
    case SpiPeripheral::kSpi3:
      return (sck_ == PinName::kPB3 || sck_ == PinName::kPC10) &&
             (miso_ == PinName::kPB4 || miso_ == PinName::kPC11) &&
             (mosi_ == PinName::kPB0 || mosi_ == PinName::kPB2 ||
              mosi_ == PinName::kPB5 || mosi_ == PinName::kPC1 ||
              mosi_ == PinName::kPC12);
  }
  return false;
}

}  // namespace nkrc::nbed::stm32::f4

#endif
