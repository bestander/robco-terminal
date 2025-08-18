// Add LVGL and LCD panel initialization logic here
#include "robco_display_component.h" // For BSP_LCD_* macros

// Add any required ESP-IDF includes for LCD and LVGL
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
#include "esp_log.h"

/* LCD settings */
#define APP_LCD_LVGL_FULL_REFRESH (0)
#define APP_LCD_LVGL_DIRECT_MODE (1)
#define APP_LCD_LVGL_AVOID_TEAR (1)
#define APP_LCD_RGB_BOUNCE_BUFFER_MODE (1)
#define APP_LCD_DRAW_BUFF_DOUBLE (0)
#define APP_LCD_DRAW_BUFF_HEIGHT (100)
#define APP_LCD_RGB_BUFFER_NUMS (2)
#define APP_LCD_RGB_BOUNCE_BUFFER_HEIGHT (10)
// ...existing code...
#include "crt_terminal_renderer.h"
// note, removed .static_bitmap field from the structure to compile
#include "FSEX302.c"
extern "C"
{
#include "lvgl.h"
}

namespace esphome
{
    namespace robco_display
    {
        CRTTerminalRenderer::CRTTerminalRenderer() {}

        void CRTTerminalRenderer::init()
        {
            static const char *TAG = "CRTTerminalRenderer";
            // LCD panel init
            esp_lcd_panel_handle_t lcd_panel;
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
            if (esp_lcd_new_rgb_panel(&conf, &lcd_panel) != ESP_OK)
            {
                ESP_LOGE(TAG, "RGB init failed");
                return;
            }
            if (esp_lcd_panel_init(lcd_panel) != ESP_OK)
            {
                ESP_LOGE(TAG, "LCD init failed");
                esp_lcd_panel_del(lcd_panel);
                return;
            }

            // LVGL port init
            const lvgl_port_cfg_t lvgl_cfg = {
                .task_priority = 4,
                .task_stack = 8192,
                .task_affinity = -1,
                .task_max_sleep_ms = 500,
                .timer_period_ms = 5};
            if (lvgl_port_init(&lvgl_cfg) != ESP_OK)
            {
                ESP_LOGE(TAG, "LVGL port initialization failed");
                return;
            }
            uint32_t buff_size = BSP_LCD_H_RES * 100;
            const lvgl_port_display_cfg_t disp_cfg = {
                .panel_handle = lcd_panel,
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
            lv_display_t *lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
            // Optionally store lvgl_disp in CRTTerminalRenderer if needed
        }

        void CRTTerminalRenderer::render(const std::vector<std::string> &lines, bool is_menu)
        {
            // LVGL lock/unlock and rendering logic moved here for swappable renderer
            lvgl_port_lock(0);
            // Clear screen to black
            lv_obj_t *scr = lv_scr_act();
            lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
            lv_obj_clean(scr);
            // Set up style for bright green text, larger font
            static lv_style_t style;
            lv_style_init(&style);
            lv_style_set_text_color(&style, lv_color_make(0, 255, 0));
            // Use the FSEX302 font (fixedsys)
            lv_style_set_text_font(&style, &fixedsys);
            int left_margin = is_menu ? 50 : 10;
            int top_margin = is_menu ? 80 : 0;
            int y = top_margin;
            for (const auto &line : lines)
            {
                lv_obj_t *label = lv_label_create(scr);
                lv_obj_add_style(label, &style, 0);
                lv_label_set_text(label, line.c_str());
                lv_obj_set_pos(label, left_margin, y);
                y += 22;
            }
            lvgl_port_unlock();
        }

    } // namespace robco_display
} // namespace esphome