#include "arduino_gfx_display.h"
#include "esphome/core/log.h"

namespace esphome {
namespace arduino_gfx_display {

void ArduinoGFXDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Arduino GFX Display...");
  
  // Copy exactly from HelloWorld.ino
  // Define a data bus (e.g., SPI)
  this->bus_ = new Arduino_SWSPI(
      GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */
  );

  // Define the RGB panel
  this->rgbpanel_ = new Arduino_ESP32RGBPanel(
      41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
      9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
      15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */,
      0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
      0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 16000000 /* prefer_speed */
  );

  // Define the display with the ST7701 driver (adjust init operations as needed)
  this->gfx_ = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, this->rgbpanel_, 0 /* rotation */, true /* auto_flush */,
      this->bus_, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
  );
  
  // Copy HelloWorld.ino setup
  this->gfx_->begin();
  this->gfx_->fillScreen(BLACK);

  // Backlight
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  this->gfx_->setCursor(10, 10);
  this->gfx_->setTextColor(RED);
  this->gfx_->println("Hello World!");

  delay(2000);
  
  this->setup_complete_ = true;
  ESP_LOGCONFIG(TAG, "Arduino GFX Display setup complete!");
}

void ArduinoGFXDisplay::loop() {
  if (!this->setup_complete_ || this->gfx_ == nullptr) {
    return;
  }
  
  // Copy HelloWorld.ino loop - animate hello world every second
  uint32_t now = millis();
  if (now - this->last_update_ > 1000) {
    this->gfx_->setCursor(random(this->gfx_->width()), random(this->gfx_->height()));
    this->gfx_->setTextColor(random(0xffff), random(0xffff));
    this->gfx_->setTextSize(random(6), random(6), random(2));
    this->gfx_->println("Hello World!");
    this->last_update_ = now;
  }
}

void ArduinoGFXDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Arduino GFX Display:");
  ESP_LOGCONFIG(TAG, "  Display Status: %s", this->gfx_ ? "OK" : "FAILED");
}

}  // namespace arduino_gfx_display
}  // namespace esphome
