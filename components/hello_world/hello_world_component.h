#ifndef HELLO_WORLD_COMPONENT_H
#define HELLO_WORLD_COMPONENT_H

#include "esphome.h"
#include "esp_lcd_panel_rgb.h"

namespace esphome {
namespace hello_world {

class HelloWorldComponent : public Component {
 public:
  HelloWorldComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

 private:
  esp_lcd_panel_handle_t panel_handle_ = nullptr;
  uint16_t *framebuffer_ = nullptr;
  void draw_text(int x, int y, const char *text, uint16_t color);
};

}  // namespace hello_world
}  // namespace esphome

#endif