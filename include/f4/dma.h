#pragma once

#include "interface/dma.h"

#include "stm32f4xx_hal.h"  // IWYU pragma: export
#include "stm32f4xx_hal_dma.h"

namespace nbed::f4 {

// For Tim1Ch1
class Dma1 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim1Ch2
class Dma2 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim1Ch3
class Dma3 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim1Ch4
class Dma4 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim2Ch1
class Dma5 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim2Ch2
class Dma6 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

// For Tim2Ch3
class Dma7 final : public interface::Dma {
 public:
  void Initialize() override;

  static DMA_HandleTypeDef& GetHandle();

 private:
  static constinit DMA_HandleTypeDef hdma_;
};

class DmaManager {
 public:
  DmaManager() = delete;

  static void Initialize();

  static Dma1& GetDma1();

  static Dma2& GetDma2();

  static Dma3& GetDma3();

  static Dma4& GetDma4();

  static Dma5& GetDma5();

  static Dma6& GetDma6();

  static Dma7& GetDma7();

 private:
  static constinit Dma1 dma1_;
  static constinit Dma2 dma2_;
  static constinit Dma3 dma3_;
  static constinit Dma4 dma4_;
  static constinit Dma5 dma5_;
  static constinit Dma6 dma6_;
  static constinit Dma7 dma7_;
};

}  // namespace nbed::f4
