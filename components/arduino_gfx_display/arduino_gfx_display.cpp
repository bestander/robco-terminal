#include "arduino_gfx_display.h"
#include "esphome/core/log.h"

// Include Arduino_GFX only in implementation to avoid macro conflicts
#include <Wire.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

namespace esphome {
namespace arduino_gfx_display {

void ArduinoGFXDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Arduino GFX Display...");
  
  // Pin configuration - direct from HelloWorld.ino
  #define GFX_BL 2
  #define TFT_BL 2

  #define GFX_BUS_END -1

  #define GFX_DEV_DEVICE ESP32_DISPLAY_7INCH
  #define GFX_BL 2

  // Configure the data bus first
  auto *bus = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
    0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

  // Store the bus
  this->bus_ = bus;

  // Configure the RGB display
  auto *display = new Arduino_RGB_Display(
    800 /* width */, 480 /* height */, bus);
    
  // Store the display
  this->gfx_ = display;

  // Begin the display
  display->begin();
  display->fillScreen(0x0000);  // Black
  
  #ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
  #endif

  // Initial setup - white text with size 2
  display->setTextColor(0xFFFF);  // White
  display->setTextSize(2);
  
  this->setup_complete_ = true;
  ESP_LOGCONFIG(TAG, "Arduino GFX Display setup complete");
}

void ArduinoGFXDisplay::loop() {
  if (!this->setup_complete_ || this->gfx_ == nullptr) {
    return;
  }
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Animation timing - update every 100ms like HelloWorld.ino
  uint32_t now = millis();
  if (now - this->last_update_ < 100) {
    return;
  }
  this->last_update_ = now;

  // Simple animation from HelloWorld.ino
  static int16_t x = 0;
  static int16_t y = 0;
  static int16_t w = 800;
  static int16_t h = 480;
  static char hello[] = "Hello World!";
  static uint8_t color = 0;

  if (x + 12 * 12 >= w) {  // rough text width calculation
    x = 0;
    y += 20;
    if (y >= h) {
      y = 0;
      display->fillScreen(0x0000);  // Clear screen
    }
  }

  // Cycle through colors - using numeric RGB565 values to avoid macro conflicts
  uint16_t text_color;
  switch (color % 7) {
    case 0: text_color = 0xF800; break;  // Red
    case 1: text_color = 0x07E0; break;  // Green  
    case 2: text_color = 0x001F; break;  // Blue
    case 3: text_color = 0xFFE0; break;  // Yellow
    case 4: text_color = 0xF81F; break;  // Magenta
    case 5: text_color = 0x07FF; break;  // Cyan
    default: text_color = 0xFFFF; break; // White
  }
  
  display->setTextColor(text_color);
  display->setCursor(x, y);
  display->print(hello);
  
  x += 120; // Move to next position
  if (x + 120 >= w) {
    color++;
  }
}

void ArduinoGFXDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Arduino GFX Display:");
  ESP_LOGCONFIG(TAG, "  Setup: %s", this->setup_complete_ ? "Complete" : "Pending");
}

}  // namespace arduino_gfx_display
}  // namespace esphome
