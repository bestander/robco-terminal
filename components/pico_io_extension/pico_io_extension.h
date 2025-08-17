#pragma once
#include "esphome/core/component.h"
#include <functional>

namespace esphome {
namespace pico_io_extension {

class PicoIOExtension : public esphome::Component {
 public:
  void set_uart_pins(int rx, int tx);
  void set_key_press_callback(std::function<void(uint8_t keycode, uint8_t modifiers)> cb);
  void setPin(uint8_t pin, bool state);
  void setup() override;
  void loop() override;

 private:
  int rx_pin_ = 17;
  int tx_pin_ = 18;
  std::function<void(uint8_t, uint8_t)> key_press_cb_ = nullptr;
};

}  // namespace pico_io_extension
}  // namespace esphome
