// File: display_manager.cpp
#include "display_manager.h"
#include "esphome/core/log.h"
#include <Wire.h>
#include <SPI.h>

namespace esphome {
namespace robco_terminal {

static const char *TAG = "robco_terminal.display";

DisplayManager::DisplayManager() : bus_(nullptr), rgbpanel_(nullptr), gfx_(nullptr) {}

DisplayManager::~DisplayManager() {
  delete gfx_;
  delete rgbpanel_;
  delete bus_;
}

void DisplayManager::init() {
  ESP_LOGCONFIG(TAG, "Initializing Arduino_GFX display for RobCo Terminal...");

  #define TFT_BL 2

  // Define a data bus (e.g., SPI) - exact from HelloWorld.ino
  bus_ = new Arduino_SWSPI(
      GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */
  );

  // Define the RGB panel - optimized timing for stability and WiFi coexistence
  rgbpanel_ = new Arduino_ESP32RGBPanel(
      41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
      9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
      15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */,
      0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
      0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 10000000 /* prefer_speed - reduced from 12MHz to 10MHz for WiFi coexistence */
  );

  // Define the display with the ST7701 driver - match Arduino version timing
  gfx_ = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, rgbpanel_, 0 /* rotation */, false /* auto_flush - manual control like Arduino */,
      bus_, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
  );

  // Begin the display with exact Arduino timing
  gfx_->begin();
  delay(100); // Longer stabilization delay
  gfx_->fillScreen(BLACK);
  delay(50); // Let clear settle

  #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    delay(20);
  #endif

  ESP_LOGCONFIG(TAG, "Arduino_GFX display initialized with optimized timing!");
}

void DisplayManager::fillScreen(uint32_t color) {
  if (!gfx_) return;
  gfx_->fillScreen(rgb888_to_rgb565(color));
}

void DisplayManager::drawText(int x, int y, const std::string &text, uint32_t color) {
  if (!gfx_ || text.empty()) return;

  // Use default green color if not specified (Fallout terminal style)
  if (color == 0) color = 0x00FF00; // Bright green

  // Convert RGB888 to RGB565 for Arduino_GFX
  uint16_t rgb565_color = rgb888_to_rgb565(color);

  // Set cursor and color
  gfx_->setCursor(x, y);
  gfx_->setTextColor(rgb565_color);

  // Use larger text size for authentic terminal feel (2x size)
  gfx_->setTextSize(2, 2);

  // Print the text
  gfx_->print(text.c_str());
}

void DisplayManager::drawChar(int x, int y, char c, uint32_t color) {
  if (!gfx_) return;

  // Use default green color if not specified
  if (color == 0) color = 0x00FF00; // Bright green

  // Convert RGB888 to RGB565 for Arduino_GFX
  uint16_t rgb565_color = rgb888_to_rgb565(color);

  // Set cursor and color
  gfx_->setCursor(x, y);
  gfx_->setTextColor(rgb565_color);

  // Use larger text size to match draw_text (2x size)
  gfx_->setTextSize(2, 2);

  // Print the character
  gfx_->print(c);
}

void DisplayManager::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if (!gfx_) return;
  gfx_->drawFastHLine(x, y, w, color);
}

void DisplayManager::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if (!gfx_) return;
  gfx_->drawFastVLine(x, y, h, color);
}

void DisplayManager::flush() {
  if (!gfx_) return;
  gfx_->flush();
}

void DisplayManager::setTextSize(uint8_t sx, uint8_t sy) {
  if (!gfx_) return;
  gfx_->setTextSize(sx, sy);
}

void DisplayManager::renderScanLines() {
  if (!gfx_) return;

  // Draw lighter scan lines for authentic CRT effect
  // Use a very dark green for subtle scan lines
  uint16_t scan_line_color = rgb888_to_rgb565(0x001100); // Very dark green

  // Draw horizontal scan lines every 4 pixels for subtle effect (less intensive)
  for (int y = 2; y < 480; y += 4) {
    gfx_->drawFastHLine(0, y, 800, scan_line_color);
  }

  // Reduce interference lines frequency to prevent timing issues
  static uint32_t last_interference = 0;
  uint32_t now = millis();

  // Add random vertical interference lines less frequently for authentic feel
  if ((now - last_interference) > 10000) { // Every 10 seconds instead of 5
    for (int i = 0; i < 2; i++) { // Fewer lines
      int x = random(800);
      uint16_t interference_color = rgb888_to_rgb565(0x002200); // Slightly brighter dark green
      gfx_->drawFastVLine(x, 0, 480, interference_color);
    }
    last_interference = now;
  }
}

void DisplayManager::clearScreen() {
  fillScreen(0x000000);  // Black
}

uint16_t DisplayManager::rgb888_to_rgb565(uint32_t rgb888) {
  uint8_t r = (rgb888 >> 16) & 0xFF;
  uint8_t g = (rgb888 >> 8) & 0xFF;
  uint8_t b = rgb888 & 0xFF;

  // Convert to RGB565: 5 bits red, 6 bits green, 5 bits blue
  uint16_t r565 = (r >> 3) & 0x1F;
  uint16_t g565 = (g >> 2) & 0x3F;
  uint16_t b565 = (b >> 3) & 0x1F;

  return (r565 << 11) | (g565 << 5) | b565;
}

}  // namespace robco_terminal
}  // namespace esphome