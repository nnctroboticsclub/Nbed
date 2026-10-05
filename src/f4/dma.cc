#if defined(STM32F446xx)

#include "f4/dma.h"

#include "stm32f446xx.h"
#include "stm32f4xx_hal.h"  // IWYU pragma: keep
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_rcc.h"

namespace nbed::f4 {
namespace {

void HandleInterrupt(DMA_HandleTypeDef& handle) {
  // No stream is configured until a peripheral claims this handle. A pending
  // interrupt on an otherwise unused stream must not dereference its empty
  // HAL bookkeeping fields.
  if (handle.Instance != nullptr && handle.StreamBaseAddress != 0U) {
    HAL_DMA_IRQHandler(&handle);
  }
}

}  // namespace

constinit DMA_HandleTypeDef Dma1::hdma_{};
constinit DMA_HandleTypeDef Dma2::hdma_{};
constinit DMA_HandleTypeDef Dma3::hdma_{};
constinit DMA_HandleTypeDef Dma4::hdma_{};
constinit DMA_HandleTypeDef Dma5::hdma_{};
constinit DMA_HandleTypeDef Dma6::hdma_{};
constinit DMA_HandleTypeDef Dma7::hdma_{};

void Dma1::Initialize() {
  __HAL_RCC_DMA2_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

DMA_HandleTypeDef& Dma1::GetHandle() {
  return hdma_;
}

void Dma2::Initialize() {
  __HAL_RCC_DMA2_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

DMA_HandleTypeDef& Dma2::GetHandle() {
  return hdma_;
}

void Dma3::Initialize() {
  __HAL_RCC_DMA2_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

DMA_HandleTypeDef& Dma3::GetHandle() {
  return hdma_;
}

void Dma4::Initialize() {
  __HAL_RCC_DMA2_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}

DMA_HandleTypeDef& Dma4::GetHandle() {
  return hdma_;
}

void Dma5::Initialize() {
  __HAL_RCC_DMA1_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

DMA_HandleTypeDef& Dma5::GetHandle() {
  return hdma_;
}

void Dma6::Initialize() {
  __HAL_RCC_DMA1_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

DMA_HandleTypeDef& Dma6::GetHandle() {
  return hdma_;
}

void Dma7::Initialize() {
  __HAL_RCC_DMA1_CLK_ENABLE();  // NOLINT
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

DMA_HandleTypeDef& Dma7::GetHandle() {
  return hdma_;
}

constinit Dma1 DmaManager::dma1_{};
constinit Dma2 DmaManager::dma2_{};
constinit Dma3 DmaManager::dma3_{};
constinit Dma4 DmaManager::dma4_{};
constinit Dma5 DmaManager::dma5_{};
constinit Dma6 DmaManager::dma6_{};
constinit Dma7 DmaManager::dma7_{};

void DmaManager::Initialize() {
  dma1_.Initialize();
  dma2_.Initialize();
  dma3_.Initialize();
  dma4_.Initialize();
  dma5_.Initialize();
  dma6_.Initialize();
  dma7_.Initialize();
}

Dma1& DmaManager::GetDma1() {
  return dma1_;
}

Dma2& DmaManager::GetDma2() {
  return dma2_;
}

Dma3& DmaManager::GetDma3() {
  return dma3_;
}

Dma4& DmaManager::GetDma4() {
  return dma4_;
}

Dma5& DmaManager::GetDma5() {
  return dma5_;
}

Dma6& DmaManager::GetDma6() {
  return dma6_;
}

Dma7& DmaManager::GetDma7() {
  return dma7_;
}

extern "C" {

void DMA2_Stream1_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma1::GetHandle());  // NOLINT
}

void DMA2_Stream2_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma2::GetHandle());  // NOLINT
}

void DMA2_Stream6_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma3::GetHandle());  // NOLINT
}

void DMA2_Stream4_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma4::GetHandle());  // NOLINT
}

void DMA1_Stream5_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma5::GetHandle());  // NOLINT
}

void DMA1_Stream6_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma6::GetHandle());  // NOLINT
}

void DMA1_Stream1_IRQHandler() {       // NOLINT
  HandleInterrupt(Dma7::GetHandle());  // NOLINT
}

}  // extern "C"

}  // namespace nbed::f4

#endif
