#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace arduino_gfx_display {

static const char *const TAG = "arduino_gfx_display";

class ArduinoGFXDisplay : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
 private:
  void initialize_display();
  
 protected:
  // Use void pointers to avoid including Arduino_GFX headers here
  void *bus_{nullptr};
  void *rgbpanel_{nullptr};
  void *gfx_{nullptr};
  
  // Animation state
  uint32_t last_update_{0};
};

}  // namespace arduino_gfx_display
}  // namespace esphome
