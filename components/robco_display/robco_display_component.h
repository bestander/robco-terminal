#include "esphome/core/automation.h"
#pragma once
#include "esphome/core/component.h"
#include "../pico_io_extension/pico_io_extension.h"
#include "menu_state.h"
#include "crt_terminal_renderer.h"
extern "C"
{
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"
#include "lv_examples.h"
#include "lv_demos.h"
}

namespace esphome
{
    namespace robco_display
    {

        class RobcoDisplayComponent : public esphome::Component
        {
        public:
                void setup() override;
                void loop() override;
                void set_pico_io_extension(esphome::pico_io_extension::PicoIOExtension *ext);
                void on_key_press(uint8_t keycode, uint8_t modifiers);
                void set_pin(uint8_t pin, bool state);
                void set_vault_door_state(const std::string &state);
                void set_red_light_pin(int pin) { red_light_pin_ = pin; }
                void set_green_light_pin(int pin) { green_light_pin_ = pin; }

    private:
            esphome::pico_io_extension::PicoIOExtension *pico_io_ext_ = nullptr;
            esp_lcd_panel_handle_t lcd_panel = nullptr;
            i2c_master_bus_handle_t my_bus = nullptr;
            lv_display_t *lvgl_disp = nullptr;
            MenuState menu_state_;
            CRTTerminalRenderer crt_renderer;
            esp_err_t app_lcd_init(esp_lcd_panel_handle_t *lp);
            esp_err_t app_lvgl_init(esp_lcd_panel_handle_t lp,
                                    lv_display_t **lv_disp);
            void render_menu();
            std::string vault_door_state_ = "Unknown";
            std::vector<MenuEntry> menu_;
            // LED blink state
            uint32_t get_millis();
            void handle_blink();
            int blink_active_ = 0; // 0 means inactive, otherwise pin number
            int red_light_pin_ = 17;
            int green_light_pin_ = 21;
            uint32_t blink_start_ms_ = 0;
            uint32_t last_blink_ms_ = 0;
            bool led_state_ = false;
        };
    } // namespace robco_display
} // namespace esphome
