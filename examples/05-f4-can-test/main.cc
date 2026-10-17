#include "logger.h"
#include "nbed.h"

void OnRxMessage(const CanMessage& message) {
  Logger::Value("can",
                "received message(id:0x%X, size:%u, data:%02X %02X %02X "
                "%02X %02X %02X %02X %02X,)",
                message.id, message.size, message.data.at(0),
                message.data.at(1), message.data.at(2), message.data.at(3),
                message.data.at(4), message.data.at(5), message.data.at(6),
                message.data.at(7));
}

int main() {
  System::Initialize();

  Can can1(kCan1, kPB8, kPB9, 1'000'000);
  Can can2(kCan2, kPB5, kPB6, 1'000'000);

  can1.Initialize();
  can2.Initialize();

  can2.SetRxCallback(OnRxMessage);
  can2.EnableRxInterrupt();

  can1.Start();
  can2.Start();

  CanMessage tx{
      .id = 0x123,
      .size = 8,
      .data = {0x4E, 0x42, 0x45, 0x44, 0x20, 0x43, 0x41, 0x4E},
  };

  while (true) {
    can1.Send(tx);
  }
}
