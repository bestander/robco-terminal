#include "hello_world_component.h"
#include "esphome/core/log.h"
#include "st7701_init.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>

// Simple 8x8 bitmap font (ASCII subset for "Hello, World!")
static const uint8_t font_8x8[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space (0x20)
    {0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00}, // '!' (0x21)
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x08, 0x00}, // ',' (0x2C)
    {0x7C, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00}, // 'H' (0x48)
    {0x00, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00, 0x00}, // 'e' (0x65)
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x3C, 0x00, 0x00}, // 'l' (0x6C)
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00}, // 'o' (0x6F)
    {0x44, 0x44, 0x44, 0x54, 0x54, 0x28, 0x00, 0x00}, // 'W' (0x57)
    {0x00, 0x58, 0x24, 0x20, 0x20, 0x70, 0x00, 0x00}, // 'r' (0x72)
    {0x04, 0x04, 0x3C, 0x44, 0x44, 0x3C, 0x00, 0x00}, // 'd' (0x64)
};

namespace esphome {
namespace hello_world {
HelloWorldComponent::HelloWorldComponent() {
  ESP_LOGD("hello_world", "HelloWorldComponent constructor called");
}

static const char *TAG = "hello_world";

void HelloWorldComponent::setup() {
  ESP_LOGD(TAG, "HelloWorldComponent: Entering setup...");
  ESP_LOGD(TAG, "Starting ST7701 RGB display setup...");

  // Configure RGB panel (C++ style, positional initialization)
  esp_lcd_rgb_panel_config_t panel_config = {};
  panel_config.data_width = 16;
  panel_config.num_fbs = 2;
  panel_config.bounce_buffer_size_px = 10 * 800;
  panel_config.clk_src = LCD_CLK_SRC_PLL160M;
  panel_config.timings.pclk_hz = 10000000;
  panel_config.timings.h_res = 800;
  panel_config.timings.v_res = 480;
  panel_config.timings.hsync_pulse_width = 30;
  panel_config.timings.hsync_back_porch = 16;
  panel_config.timings.hsync_front_porch = 210;
  panel_config.timings.vsync_pulse_width = 13;
  panel_config.timings.vsync_back_porch = 10;
  panel_config.timings.vsync_front_porch = 22;
  panel_config.timings.flags.pclk_active_neg = 1;
  panel_config.flags.fb_in_psram = 1;
  panel_config.hsync_gpio_num = 39;
  panel_config.vsync_gpio_num = 40;
  panel_config.de_gpio_num = 41;
  panel_config.pclk_gpio_num = 42;
  int data_gpios[16] = {14, 21, 47, 48, 45, 9, 46, 3, 8, 16, 1, 15, 7, 6, 5, 4};
  memcpy(panel_config.data_gpio_nums, data_gpios, sizeof(data_gpios));
  panel_config.disp_gpio_num = GPIO_NUM_NC;

  ESP_LOGD(TAG, "Creating RGB panel...");
  esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGD(TAG, "Resetting panel...");
  err = esp_lcd_panel_reset(panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to reset panel: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGD(TAG, "Initializing panel...");
  err = esp_lcd_panel_init(panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize panel: %s", esp_err_to_name(err));
    return;
  }

  // SPI for ST7701 control (standard SPI, C++ style)
  ESP_LOGD(TAG, "Setting up SPI for ST7701...");
  esp_lcd_panel_io_handle_t io_handle;
  esp_lcd_panel_io_spi_config_t io_config = {};
  io_config.cs_gpio_num = 10;
  io_config.dc_gpio_num = 48; // Data/Command pin, adjust as needed
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  io_config.spi_mode = 0;
  io_config.trans_queue_depth = 10;
  // You must create SPI bus first and get its handle (spi_bus_handle)
  // Example: esp_lcd_spi_bus_handle_t spi_bus_handle = ...;
  // err = esp_lcd_new_panel_io_spi(spi_bus_handle, &io_config, &io_handle);
  // For now, this is a placeholder. You must implement SPI bus creation before this call.

  ESP_LOGD(TAG, "Sending ST7701 init commands...");
  err = esp_lcd_panel_io_tx_param(io_handle, 0x00, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send ST7701 init commands: %s", esp_err_to_name(err));
    return;
  }

  // Backlight
  ESP_LOGD(TAG, "Setting up backlight on GPIO 2...");
  err = gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set backlight GPIO direction: %s", esp_err_to_name(err));
    return;
  }
  err = gpio_set_level(GPIO_NUM_2, 1);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set backlight GPIO level: %s", esp_err_to_name(err));
    return;
  }

  // Allocate framebuffer in PSRAM
  ESP_LOGD(TAG, "Allocating framebuffer in PSRAM...");
  framebuffer_ = (uint16_t *)heap_caps_calloc(800 * 480, sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (!framebuffer_) {
    ESP_LOGE(TAG, "Failed to allocate framebuffer in PSRAM");
    return;
  }

  // Clear screen to black
  ESP_LOGD(TAG, "Clearing screen to black...");
  for (int i = 0; i < 800 * 480; i++) {
    framebuffer_[i] = 0x0000;  // Black in RGB565
  }
  err = esp_lcd_panel_draw_bitmap(panel_handle_, 0, 0, 800, 480, framebuffer_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to draw bitmap: %s", esp_err_to_name(err));
    return;
  }

  // Draw "Hello, World!" in green
  ESP_LOGD(TAG, "Drawing 'Hello, World!'...");
  draw_text(400 - 6 * 13, 240 - 4, "Hello, World!", 0x07E0);

  ESP_LOGI(TAG, "ST7701 RGB display initialized with 'Hello, World!'");
}

void HelloWorldComponent::loop() {
  ESP_LOGD(TAG, "HelloWorldComponent: In loop...");
  // No continuous updates needed
}

void HelloWorldComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Hello World Component: Configured for 800x480, 10MHz PCLK");
}

void HelloWorldComponent::draw_text(int x, int y, const char *text, uint16_t color) {
  if (!framebuffer_ || !text) {
    ESP_LOGE(TAG, "Invalid framebuffer or text pointer");
    return;
  }

  int cursor_x = x;
  for (const char *c = text; *c; c++) {
    if (*c >= 0x20 && *c < 0x80) {
      const uint8_t *glyph = font_8x8[*c - 0x20];
      for (int gy = 0; gy < 8; gy++) {
        for (int gx = 0; gx < 8; gx++) {
          if (glyph[gy] & (1 << (7 - gx))) {
            int px = cursor_x + gx;
            int py = y + gy;
            if (px >= 0 && px < 800 && py >= 0 && py < 480) {
              framebuffer_[py * 800 + px] = color;
            }
          }
        }
      }
      cursor_x += 8;
    }
  }
  esp_err_t err = esp_lcd_panel_draw_bitmap(panel_handle_, x, y, x + 13 * 8, y + 8, framebuffer_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to draw text bitmap: %s", esp_err_to_name(err));
  }
}

}  // namespace hello_world
}  // namespace esphome
