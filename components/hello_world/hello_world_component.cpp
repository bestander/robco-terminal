// ...existing code...
#include "hello_world_component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hello_world {

static const char *TAG = "HelloWorldComponent";

void HelloWorldComponent::set_pico_io_extension(esphome::pico_io_extension::PicoIOExtension* ext) {
    pico_io_ext_ = ext;
    if (pico_io_ext_) {
        pico_io_ext_->set_key_press_callback([this](uint8_t keycode, uint8_t modifiers) {
            this->on_key_press(keycode, modifiers);
        });
    }
}

void HelloWorldComponent::on_key_press(uint8_t keycode, uint8_t modifiers) {
    ESP_LOGI(TAG, "HelloWorld received key press: code=0x%02X, modifiers=0x%02X", keycode, modifiers);
    // Example: toggle pin 21 on 'a' (0x04)
    if (keycode == 0x04 && pico_io_ext_) {
        static bool pin21_state = false;
        pin21_state = !pin21_state;
        pico_io_ext_->setPin(21, pin21_state);
    }
    if (key_label_) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Key: 0x%02X", keycode);
        lvgl_port_lock(0);
        lv_label_set_text(key_label_, buf);
        lvgl_port_unlock();
    }
}

void HelloWorldComponent::set_pin(uint8_t pin, bool state) {
    if (pico_io_ext_) pico_io_ext_->setPin(pin, state);
}

/* LCD settings */
#define APP_LCD_LVGL_FULL_REFRESH           (0)
#define APP_LCD_LVGL_DIRECT_MODE            (1)
#define APP_LCD_LVGL_AVOID_TEAR             (1)
#define APP_LCD_RGB_BOUNCE_BUFFER_MODE      (1)
#define APP_LCD_DRAW_BUFF_DOUBLE            (0)
#define APP_LCD_DRAW_BUFF_HEIGHT            (100)
#define APP_LCD_RGB_BUFFER_NUMS             (2)
#define APP_LCD_RGB_BOUNCE_BUFFER_HEIGHT    (10)

esp_err_t HelloWorldComponent::app_lcd_init(esp_lcd_panel_handle_t *lp) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Initialize RGB panel");
    const esp_lcd_rgb_panel_config_t conf = {
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .timings = BSP_LCD_PANEL_TIMING(),
    .data_width = 16,
    .num_fbs = APP_LCD_RGB_BUFFER_NUMS,
#ifdef APP_LCD_RGB_BOUNCE_BUFFER_MODE
    .bounce_buffer_size_px = BSP_LCD_H_RES * APP_LCD_RGB_BOUNCE_BUFFER_HEIGHT,
#endif
    .hsync_gpio_num = BSP_LCD_GPIO_HSYNC,
    .vsync_gpio_num = BSP_LCD_GPIO_VSYNC,
    .de_gpio_num = BSP_LCD_GPIO_DE,
    .pclk_gpio_num = BSP_LCD_GPIO_PCLK,
    .disp_gpio_num = BSP_LCD_GPIO_DISP,
    .data_gpio_nums = BSP_LCD_GPIO_DATA(),
    .flags = { .fb_in_psram = 1 },
    };
    if (esp_lcd_new_rgb_panel(&conf, lp) != ESP_OK) {
        ESP_LOGE(TAG, "RGB init failed");
        return ESP_FAIL;
    }
    if (esp_lcd_panel_init(*lp) != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed");
        esp_lcd_panel_del(*lp);
        return ESP_FAIL;
    }
    return ret;
}

esp_err_t esphome::hello_world::HelloWorldComponent::app_touch_init(i2c_master_bus_handle_t *bus,
                                              esp_lcd_panel_io_handle_t *tp_io,
                                              esp_lcd_touch_handle_t *tp) {
    if (!*bus) {
        ESP_LOGI(TAG, "creating i2c master bus");
        const i2c_master_bus_config_t i2c_conf = {
            .i2c_port = -1,
            .sda_io_num = BSP_TOUCH_GPIO_SDA,
            .scl_io_num = BSP_TOUCH_GPIO_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = {.enable_internal_pullup = 1},
        };
        if (i2c_new_master_bus(&i2c_conf, bus) != ESP_OK) {
            ESP_LOGE(TAG, "failed to create i2c master bus");
            return ESP_FAIL;
        }
    }
    if (!*tp_io) {
        ESP_LOGI(TAG, "creating touch panel io");
        esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        tp_io_cfg.scl_speed_hz = 400000;
        if (esp_lcd_new_panel_io_i2c_v2(*bus, &tp_io_cfg, tp_io) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create touch panel io");
            return ESP_FAIL;
        }
    }
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_TOUCH_GPIO_RST,
        .int_gpio_num = BSP_TOUCH_GPIO_INT,
    };
    return esp_lcd_touch_new_i2c_gt911(*tp_io, &tp_cfg, tp);
}

esp_err_t esphome::hello_world::HelloWorldComponent::app_lvgl_init(esp_lcd_panel_handle_t lp, esp_lcd_touch_handle_t tp,
                                             lv_display_t **lv_disp, lv_indev_t **lv_touch_indev) {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port initialization failed");
        return ESP_FAIL;
    }
    uint32_t buff_size = BSP_LCD_H_RES * 100;
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = lp,
        .buffer_size = buff_size,
        .double_buffer = 0,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .rotation = {.swap_xy = false, .mirror_x = false, .mirror_y = false},
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = true
        }
    };
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {.bb_mode = true, .avoid_tearing = true}
    };
    *lv_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = *lv_disp,
        .handle = tp,
    };
    *lv_touch_indev = lvgl_port_add_touch(&touch_cfg);
    return ESP_OK;
}

void esphome::hello_world::HelloWorldComponent::setup() {
    ESP_LOGI(TAG, "Setting up HelloWorldComponent");
    ESP_ERROR_CHECK(app_lcd_init(&lcd_panel));
    ESP_ERROR_CHECK(app_touch_init(&my_bus, &touch_io_handle, &touch_handle));
    ESP_ERROR_CHECK(app_lvgl_init(lcd_panel, touch_handle, &lvgl_disp, &lvgl_touch_indev));
    const gpio_config_t bk_light = {
        .pin_bit_mask = (1 << BSP_LCD_GPIO_BK_LIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_light));
    gpio_set_level(BSP_LCD_GPIO_BK_LIGHT, BSP_LCD_BK_LIGHT_ON_LEVEL);
    lvgl_port_lock(0);
    key_label_ = lv_label_create(lv_scr_act());
    lv_label_set_text(key_label_, "Hello World");
    lv_obj_align(key_label_, LV_ALIGN_CENTER, 0, 0);
    lvgl_port_unlock(); 
}

void esphome::hello_world::HelloWorldComponent::loop() {
    // No periodic logic needed for this example
}

}  // namespace hello_world
}  // namespace esphome