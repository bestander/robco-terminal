#pragma once
#include "esphome/core/component.h"

namespace esphome {
namespace uart_logger {

class UARTLogger : public Component {
 public:
  UARTLogger();
  void setup() override;
  void loop() override;
};

}  // namespace uart_logger
}  // namespace esphome