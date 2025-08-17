#pragma once
#include "esphome/core/component.h"
#include "../pico_io_extension/pico_io_extension.h"

namespace esphome {
namespace robco_display {

class PicoIOExtensionListener {
 public:
  virtual void on_key_press(uint8_t keycode, uint8_t modifiers) = 0;
};


}  // namespace robco_display
}  // namespace esphome
