#pragma once
#include "esphome/core/component.h"
extern "C" {
#include "bsp.h"
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"
#include "lv_examples.h"
#include "lv_demos.h"
}

namespace esphome {
namespace hello_world {

class HelloWorldComponent : public esphome::Component {
public:
  void setup() override;
  void loop() override;
private:
  esp_lcd_panel_handle_t lcd_panel = nullptr;
  i2c_master_bus_handle_t my_bus = nullptr;
  esp_lcd_panel_io_handle_t touch_io_handle = nullptr;
  esp_lcd_touch_handle_t touch_handle = nullptr;
  lv_display_t *lvgl_disp = nullptr;
  lv_indev_t *lvgl_touch_indev = nullptr;
  esp_err_t app_lcd_init(esp_lcd_panel_handle_t *lp);
  esp_err_t app_touch_init(i2c_master_bus_handle_t *bus,
                          esp_lcd_panel_io_handle_t *tp_io,
                          esp_lcd_touch_handle_t *tp);
  esp_err_t app_lvgl_init(esp_lcd_panel_handle_t lp, esp_lcd_touch_handle_t tp,
                         lv_display_t **lv_disp, lv_indev_t **lv_touch_indev);
};


}  // namespace hello_world
}  // namespace esphome
