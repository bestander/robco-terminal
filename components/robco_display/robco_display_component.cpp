#include "robco_display_component.h"
#include "crt_terminal_renderer.h"
#include "esphome/core/log.h"
#include "esp_timer.h"
#include "esphome/components/mqtt/mqtt_client.h"

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

        // Helper to get current time in milliseconds (replace with ESP-IDF API if needed)
        uint32_t RobcoDisplayComponent::get_millis()
        {
            return esp_timer_get_time() / 1000;
        }

        void RobcoDisplayComponent::on_key_press(uint8_t keycode, uint8_t modifiers)
        {
            ESP_LOGI(TAG, "RobcoDisplay received key press: code=0x%02X, modifiers=0x%02X", keycode, modifiers);
            // Save previous menu stack and selected index
            int prev_selected = menu_state_.get_selected_index();
            std::vector<MenuEntry> *current_menu = &menu_;
            const auto &menu_stack = menu_state_.get_menu_stack();
            for (size_t i = 0; i < menu_stack.size(); ++i)
            {
                int idx = menu_stack[i];
                if (idx >= 0 && idx < current_menu->size())
                {
                    current_menu = &(*current_menu)[idx].subitems;
                }
            }
            // Password entry mode
            if (menu_state_.is_password_entry_mode())
            {
                if (keycode == 0x2A)
                { // Backspace
                    menu_state_.remove_password_char();
                }
                else if (keycode == 0x28)
                { // Enter
                    std::string password = menu_state_.get_password();
                    // Send MQTT with password as payload
                    ESP_LOGI(TAG, "Sending MQTT open with password: %s", password.c_str());
                    esphome::mqtt::global_mqtt_client->publish("garage/door/open", password, 0, false);
                    blink_active_ = green_light_pin_;
                    blink_start_ms_ = get_millis();
                    last_blink_ms_ = get_millis();
                    led_state_ = false;
                    set_pin(blink_active_, 0);
                    menu_state_.end_password_entry();
                }
                else if (keycode >= 0x04 && keycode <= 0x1D)
                { // a-z
                    char c = 'a' + (keycode - 0x04);
                    menu_state_.append_password_char(c);
                }
                else if (keycode >= 0x1E && keycode <= 0x27)
                { // 1-9,0
                    char c = (keycode == 0x27) ? '0' : '1' + (keycode - 0x1E);
                    menu_state_.append_password_char(c);
                }
                render_menu();
                return;
            }
            // Check if action is triggered
            if (keycode == 0x28 && prev_selected >= 0 && prev_selected < current_menu->size())
            {
                const auto &entry = (*current_menu)[prev_selected];
                if (entry.title == "Open Vault Door")
                {
                    menu_state_.start_password_entry("Enter password to open vault door:");
                }
                else if (entry.title == "Close Vault Door")
                {
                    ESP_LOGI(TAG, "Closing vault door");
                    esphome::mqtt::global_mqtt_client->publish("garage/door/close", std::string(""), 0, false);
                    blink_active_ = red_light_pin_;
                    blink_start_ms_ = get_millis();
                    last_blink_ms_ = get_millis();
                    led_state_ = false;
                    set_pin(blink_active_, 0);
                }
            }
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
            // Header lines to display across all menus
            std::vector<std::string> header_lines = {
                "        ROBCO INDUSTRIES UNIFIED OPERATING SYSTEM",
                "           COPYRIGHT 2075-2077 ROBCO INDUSTRIES",
                "",
                "                        -Server 1-",
                "Welcome, Overseer.",
                "------------------"};
            menu_state_.set_header(header_lines);
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
                "Network Interface: ONLINE",
                "Security Protocols: ACTIVE",
                "",
                "System Status: NOMINAL",
                "Security Level: AUTHORIZED",
                "Access Level: OVERSEER",
                "",
                "Welcome to RobCo Termlink",
                "Have a Nice Day!",
                "",
                "> Press any key to continue..."};
            menu_state_.set_boot_messages(boot_msgs);
            // Menu structure
            menu_ = {
                {"", MenuEntry::Type::STATIC, {}, {}, ""},
                {"Vault Door Control", MenuEntry::Type::SUBMENU, {
                    {"Open Vault Door", MenuEntry::Type::ACTION, {}, {}, ""},
                    {"", MenuEntry::Type::STATIC, {}, {}, ""},
                    {"Close Vault Door", MenuEntry::Type::ACTION, {}, {}, ""}}, {}, ""},
                {"", MenuEntry::Type::STATIC, {}, {}, ""},
                {"System Status", MenuEntry::Type::SUBMENU, {{"Power: Stable", MenuEntry::Type::STATIC, {}, {}, ""}, {"", MenuEntry::Type::STATIC, {}, {}, ""}, {"Door", MenuEntry::Type::STATUS, {}, {}, vault_door_state_}, {"", MenuEntry::Type::STATIC, {}, {}, ""}, {"Security: Nominal", MenuEntry::Type::STATIC, {}, {}, ""}}, {}, ""},
                {"", MenuEntry::Type::STATIC, {}, {}, ""},
                {"Overseer Logs", MenuEntry::Type::SUBMENU, {{"Read Log Entry", MenuEntry::Type::ACTION, {}, {}, ""}, {"", MenuEntry::Type::STATIC, {}, {}, ""}, {"Add Log Entry", MenuEntry::Type::ACTION, {}, {}, ""}, {"", MenuEntry::Type::STATIC, {}, {}, ""}, {"Remove Log Entry", MenuEntry::Type::ACTION, {}, {}, ""}}, {}, ""},
                {"", MenuEntry::Type::STATIC, {}, {}, ""},
            };
            menu_state_.set_menu(menu_);
            render_menu();
        }
        void RobcoDisplayComponent::set_vault_door_state(const std::string &state)
        {
            ESP_LOGI(TAG, "MQTT update received: vault_door_state='%s'", state.c_str());
            std::string formatted_state = state;
            if (state == "opened")
            {
                formatted_state = "Opened";
            }
            else if (state == "closed")
            {
                formatted_state = "Closed";
            }
            vault_door_state_ = formatted_state;
            bool found = false;
            for (auto &entry : menu_)
            {
                if (entry.title == "System Status" && !entry.subitems.empty())
                {
                    for (auto &sub : entry.subitems)
                    {
                        if (sub.title.find("Door") != std::string::npos)
                        {
                            ESP_LOGI(TAG, "Updating menu entry '%s' with state '%s'", sub.title.c_str(), state.c_str());
                            // Replace 'Unknown' with 'Closed' or 'Opened' in the title if present
                            if (sub.title.find("Unknown") != std::string::npos)
                            {
                                sub.title = "Door";
                            }
                            sub.status_value = formatted_state;
                            found = true;
                        }
                    }
                }
            }
            if (!found)
            {
                ESP_LOGW(TAG, "Could not find 'Door' entry in System Status menu to update");
            }
            menu_state_.set_menu(menu_);
            render_menu();
        }

        void RobcoDisplayComponent::loop()
        {
            handle_blink();
        }

        void RobcoDisplayComponent::handle_blink()
        {
            // Handle LED blink on door close
            if (blink_active_ > 0)
            {
                uint32_t now = get_millis();
                if (now - blink_start_ms_ >= 5000)
                {
                    set_pin(blink_active_, 0);
                    blink_active_ = 0;
                }
                else if (now - last_blink_ms_ >= 500)
                {
                    led_state_ = !led_state_;
                    set_pin(blink_active_, led_state_ ? 1 : 0);
                    last_blink_ms_ = now;
                }
            }
        }

        // Per-line cache for partial redraw
        static std::vector<std::string> cached_lines;
        void RobcoDisplayComponent::render_menu()
        {
            size_t kNumLines = this->crt_renderer.get_num_lines();
            std::string text = menu_state_.get_display_text();
            std::vector<std::string> lines;
            size_t pos = 0, prev = 0;
            while ((pos = text.find('\n', prev)) != std::string::npos)
            {
                lines.push_back(text.substr(prev, pos - prev));
                prev = pos + 1;
            }
            if (prev < text.size())
                lines.push_back(text.substr(prev));

            // Pad or trim lines to fixed length
            if (lines.size() < kNumLines)
                lines.resize(kNumLines, "");
            else if (lines.size() > kNumLines)
                lines.resize(kNumLines);

            // Ensure cache is fixed length
            if (cached_lines.size() != kNumLines)
                cached_lines.resize(kNumLines, "");

            this->crt_renderer.lock();

            // Update or clear all lines
            for (size_t i = 0; i < kNumLines; ++i)
            {
                if (lines[i] != cached_lines[i])
                {
                    this->crt_renderer.render_line(lines[i], i, true);
                    cached_lines[i] = lines[i];
                }
                else if (lines[i].empty() && !cached_lines[i].empty())
                {
                    // Clear line if it was previously non-empty
                    this->crt_renderer.render_line("", i, true);
                    cached_lines[i] = "";
                }
            }

            this->crt_renderer.unlock();
        }

    } // namespace robco_display
} // namespace esphome
