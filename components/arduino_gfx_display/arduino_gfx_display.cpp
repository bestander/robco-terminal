#include "arduino_gfx_display.h"
#include "esphome/core/log.h"

// Include Arduino_GFX only in implementation to avoid macro conflicts
#include <Wire.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

namespace esphome {
namespace arduino_gfx_display {

void ArduinoGFXDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Arduino GFX Display setup - deferring initialization to loop()");
}

void ArduinoGFXDisplay::initialize_display() {
  ESP_LOGCONFIG(TAG, "Initializing display in loop()...");
  
  // Exact copy from HelloWorld.ino
  #define TFT_BL 2
  
  // Define a data bus (e.g., SPI) - exact from HelloWorld.ino
  auto *bus = new Arduino_SWSPI(
      GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */
  );

  // Define the RGB panel - exact from HelloWorld.ino  
  auto *rgbpanel = new Arduino_ESP32RGBPanel(
      41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
      9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
      15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */,
      0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
      0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 16000000 /* prefer_speed */
  );

  // Store the bus and panel
  this->bus_ = bus;
  this->rgbpanel_ = rgbpanel;

  // Define the display with the ST7701 driver - exact from HelloWorld.ino
  auto *display = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
      bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
  );
    
  // Store the display
  this->gfx_ = display;

  // Begin the display - exact from HelloWorld.ino
  display->begin();
  display->fillScreen(0x0000);  // BLACK
  
  #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  #endif

  // Initial setup from HelloWorld.ino
  display->setCursor(10, 10);
  display->setTextColor(0xF800);  // RED
  display->println("Hello World!");

  ESP_LOGCONFIG(TAG, "Display initialized in loop successfully!");
}

void ArduinoGFXDisplay::loop() {
  static bool display_initialized = false;
  
  // Initialize display in loop after all ESPHome components are ready
  if (!display_initialized) {
    this->initialize_display();
    display_initialized = true;
    return; // Exit early this iteration
  }
  
  if (!display_initialized || this->gfx_ == nullptr) {
    return;
  }
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Animation timing - update every 1000ms like HelloWorld.ino
  uint32_t now = millis();
  if (now - this->last_update_ < 1000) {
    return;
  }
  this->last_update_ = now;
  
  // Exact copy from HelloWorld.ino loop but using millis() for random seed
  srand(now);  // Seed random with current time
  display->setCursor(rand() % display->width(), rand() % display->height());
  display->setTextColor(rand() % 0xffff, rand() % 0xffff);
  display->setTextSize(rand() % 6 + 1, rand() % 6 + 1, rand() % 2 + 1);
  display->println("Hello World!");
}

void ArduinoGFXDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Arduino GFX Display:");
}

}  // namespace arduino_gfx_display
}  // namespace esphome
