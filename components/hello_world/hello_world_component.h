#ifndef HELLO_WORLD_COMPONENT_H
#define HELLO_WORLD_COMPONENT_H

#include "esphome.h"
#include "esp_lcd_panel_rgb.h"

namespace esphome {
namespace hello_world {

class HelloWorldComponent : public Component {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;

private:
  esp_lcd_panel_handle_t panel_handle_ = nullptr;
  uint16_t *framebuffer_ = nullptr;
  void draw_text(int x, int y, const char *text, uint16_t color);
};

}  // namespace hello_world
}  // namespace esphome

#endif