// File: display_manager.h
#ifndef ROBCO_TERMINAL_DISPLAY_MANAGER_H
#define ROBCO_TERMINAL_DISPLAY_MANAGER_H

#include <Arduino_GFX_Library.h>
#include <display/Arduino_RGB_Display.h>
#include <string>

namespace esphome {
namespace robco_terminal {

class DisplayManager {
public:
  DisplayManager();
  ~DisplayManager();

  void init();
  void fillScreen(uint32_t color);
  void drawText(int x, int y, const std::string &text, uint32_t color);
  void drawChar(int x, int y, char c, uint32_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void flush();
  void setTextSize(uint8_t sx, uint8_t sy);
  void renderScanLines();
  void clearScreen();  // Calls fillScreen with BLACK

private:
  Arduino_DataBus *bus_;
  Arduino_ESP32RGBPanel *rgbpanel_;
  Arduino_GFX *gfx_;

  // Helper function to convert RGB888 to RGB565
  static uint16_t rgb888_to_rgb565(uint32_t rgb888);
};

}  // namespace robco_terminal
}  // namespace esphome

#endif