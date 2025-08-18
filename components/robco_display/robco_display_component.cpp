#include "robco_display_component.h"
#include "crt_terminal_renderer.h"
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
            menu_state_.on_key_press(keycode);
            render_menu();
        }

        void RobcoDisplayComponent::set_pin(uint8_t pin, bool state)
        {
            if (pico_io_ext_)
                pico_io_ext_->setPin(pin, state);
        }

        void RobcoDisplayComponent::setup()
        {
            ESP_LOGI(TAG, "Setting up RobcoDisplayComponent");
            crt_renderer.init();
            const gpio_config_t bk_light = {
                .pin_bit_mask = (1 << BSP_LCD_GPIO_BK_LIGHT),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            ESP_ERROR_CHECK(gpio_config(&bk_light));
            gpio_set_level(BSP_LCD_GPIO_BK_LIGHT, BSP_LCD_BK_LIGHT_ON_LEVEL);
            // Boot messages from robco_terminal.cpp
            std::vector<std::string> boot_msgs = {
                "RobCo Industries (TM) Termlink Protocol",
                "Established 2075",
                "",
                "VAULT-TEC TERMINAL SYSTEM",
                "Initializing...",
                "",
                "Boot Sequence Started",
                "Loading System Drivers...",
                "Checking Memory Banks...",
                "Memory Test: OK",
                "8MB PSRAM Detected",
                "16MB Flash Storage Available",
                "",
                "Network Interface: ONLINE",
                "MQTT Client: CONNECTING...",
                "Home Assistant Integration: READY",
                "Security Protocols: ACTIVE",
                "",
                "System Status: NOMINAL",
                "Security Level: AUTHORIZED",
                "Access Level: OVERSEER",
                "",
                "Welcome to RobCo Termlink",
                "Have a Nice Day!",
                "",
                "> Press any key to continue..."
            };
            menu_state_.set_boot_messages(boot_msgs);
            // Menu structure
            std::vector<MenuEntry> menu = {
                {"Security", MenuEntry::Type::SUBMENU, {
                    {"Vault Door Control", MenuEntry::Type::ACTION, {}, {}, ""}
                }, {}, ""},
                {"Overseer Logs", MenuEntry::Type::SUBMENU, {
                    {"List Entries", MenuEntry::Type::LOGS, {}, {}, ""},
                    {"Add Entry", MenuEntry::Type::ACTION, {}, {}, ""},
                    {"Remove Entry", MenuEntry::Type::ACTION, {}, {}, ""}
                }, {}, ""},
                {"System Status", MenuEntry::Type::STATUS, {}, {}, "outside temperature"}
            };
            menu_state_.set_menu(menu);
            render_menu();
        }

        void RobcoDisplayComponent::loop()
        {
            // No periodic logic needed for this example
        }

        void RobcoDisplayComponent::render_menu()
        {
            std::string text = menu_state_.get_display_text();
            std::vector<std::string> lines;
            size_t pos = 0, prev = 0;
            while ((pos = text.find('\n', prev)) != std::string::npos) {
                lines.push_back(text.substr(prev, pos - prev));
                prev = pos + 1;
            }
            if (prev < text.size()) lines.push_back(text.substr(prev));
            this->crt_renderer.render(lines, true); // Pass is_menu=true for menu rendering
        }

    } // namespace robco_display
} // namespace esphome
