#include "robco_display_component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace robco_display
    {

        static const char *TAG = "RobcoDisplayComponent";

        void RobcoDisplayComponent::set_pico_io_extension(esphome::pico_io_extension::PicoIOExtension *ext)
        {
            pico_io_ext_ = ext;
            if (pico_io_ext_)
            {
                pico_io_ext_->set_key_press_callback([this](uint8_t keycode, uint8_t modifiers)
                                                     { this->on_key_press(keycode, modifiers); });
            }
        }

        void RobcoDisplayComponent::on_key_press(uint8_t keycode, uint8_t modifiers)
        {
            ESP_LOGI(TAG, "RobcoDisplay received key press: code=0x%02X, modifiers=0x%02X", keycode, modifiers);
            if (keycode == 0x04 && pico_io_ext_)
            {
                static bool pin21_state = false;
                pin21_state = !pin21_state;
                pico_io_ext_->setPin(21, pin21_state);
            }
            if (key_label_)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "Key: 0x%02X", keycode);
                lvgl_port_lock(0);
                lv_label_set_text(key_label_, buf);
                lvgl_port_unlock();
            }
        }

        void RobcoDisplayComponent::set_pin(uint8_t pin, bool state)
        {
            if (pico_io_ext_)
                pico_io_ext_->setPin(pin, state);
        }

        /* LCD settings */
        #define APP_LCD_LVGL_FULL_REFRESH (0)
        #define APP_LCD_LVGL_DIRECT_MODE (1)
        #define APP_LCD_LVGL_AVOID_TEAR (1)
        #define APP_LCD_RGB_BOUNCE_BUFFER_MODE (1)
        #define APP_LCD_DRAW_BUFF_DOUBLE (0)
        #define APP_LCD_DRAW_BUFF_HEIGHT (100)
        #define APP_LCD_RGB_BUFFER_NUMS (2)
        #define APP_LCD_RGB_BOUNCE_BUFFER_HEIGHT (10)

        esp_err_t RobcoDisplayComponent::app_lcd_init(esp_lcd_panel_handle_t *lp)
        {
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
                .flags = {.fb_in_psram = 1},
            };
            if (esp_lcd_new_rgb_panel(&conf, lp) != ESP_OK)
            {
                ESP_LOGE(TAG, "RGB init failed");
                return ESP_FAIL;
            }
            if (esp_lcd_panel_init(*lp) != ESP_OK)
            {
                ESP_LOGE(TAG, "LCD init failed");
                esp_lcd_panel_del(*lp);
                return ESP_FAIL;
            }
            return ret;
        }

        esp_err_t RobcoDisplayComponent::app_lvgl_init(esp_lcd_panel_handle_t lp,
                                                       lv_display_t **lv_disp)
        {
            const lvgl_port_cfg_t lvgl_cfg = {
                .task_priority = 4,
                .task_stack = 8192,
                .task_affinity = -1,
                .task_max_sleep_ms = 500,
                .timer_period_ms = 5};
            if (lvgl_port_init(&lvgl_cfg) != ESP_OK)
            {
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
                    .direct_mode = true},
            };
            const lvgl_port_display_rgb_cfg_t rgb_cfg = {
                .flags = {.bb_mode = true, .avoid_tearing = true}};
            *lv_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
            return ESP_OK;
        }

        void RobcoDisplayComponent::setup()
        {
            ESP_LOGI(TAG, "Setting up RobcoDisplayComponent");
            ESP_ERROR_CHECK(app_lcd_init(&lcd_panel));
            ESP_ERROR_CHECK(app_lvgl_init(lcd_panel, &lvgl_disp));
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
            lv_label_set_text(key_label_, "Robco Display");
            lv_obj_align(key_label_, LV_ALIGN_CENTER, 0, 0);
            lvgl_port_unlock();
        }

        void RobcoDisplayComponent::loop()
        {
            // No periodic logic needed for this example
        }

    } // namespace robco_display
} // namespace esphome
