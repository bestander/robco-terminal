#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/gpio.h"
#include "esphome/core/preferences.h"
#include "esphome/components/web_server_base/web_server_base.h"
// Removed display_buffer.h since we use Arduino_GFX directly
// #include "esphome/components/display/display_buffer.h"
// #include "esphome/components/mqtt/mqtt_client.h"  // Temporarily removed for debugging

// OTA Safety integration
#include "esphome/components/ota_safety/ota_safety.h"

// Direct GPIO access for USB signal detection on ESP32-8048S070N/C
#include "driver/gpio.h"

#include <vector>
#include <string>
#include <map>

namespace esphome {
namespace robco_terminal {

static const char *const TAG = "robco_terminal";

// Forward declaration for Arduino_GFX components
class RobCoTerminal;

enum class MenuItemType {
  SUBMENU,
  ACTION,
  STATUS,
  TEXT_EDITOR
};

enum class TerminalState {
  BOOTING,
  MAIN_MENU,
  SUBMENU,
  TEXT_EDITOR,
  EXECUTING_ACTION
};

struct MenuItem {
  std::string title;
  MenuItemType type;
  std::string mqtt_topic;
  std::string mqtt_payload;
  bool readonly;
  std::string condition_topic;
  std::string condition_value;
  std::string file_path;
  int max_entries;
  std::vector<MenuItem> subitems;
  std::string current_value;  // For status items
  bool visible;               // For conditional items
  
  MenuItem() : readonly(false), max_entries(100), visible(true) {}
};

struct TerminalFont {
  static const uint8_t CHAR_WIDTH = 10;
  static const uint8_t CHAR_HEIGHT = 16;
  static const uint8_t CHARS_PER_LINE = 80;  // 800px / 10px
  static const uint8_t LINES_PER_SCREEN = 30; // 480px / 16px
};

class RobCoTerminal : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  // OTA Safety integration
  void disable_for_ota();
  void enable_after_ota();
  bool is_enabled_for_ota() const { return !disabled_for_ota_; }
  
  // Configuration setters - removed set_display since we use integrated Arduino_GFX
  void set_mqtt_topic_prefix(const std::string &prefix) { this->mqtt_topic_prefix_ = prefix; }
  void set_boot_sequence(bool enabled) { this->boot_sequence_ = enabled; }
  void set_cursor_blink(bool enabled) { this->cursor_blink_ = enabled; }
  void set_font_color(uint32_t color) { this->font_color_ = color; }
  void set_background_color(uint32_t color) { this->background_color_ = color; }
  void set_usb_dp_pin(GPIOPin *pin) { this->usb_dp_pin_ = pin; }
  void set_usb_dm_pin(GPIOPin *pin) { this->usb_dm_pin_ = pin; }
  
  // Button configuration
  void set_down_button_pin(GPIOPin *pin) { this->down_button_pin_ = pin; }
  void set_enter_button_pin(GPIOPin *pin) { this->enter_button_pin_ = pin; }
  void set_back_button_pin(GPIOPin *pin) { this->back_button_pin_ = pin; }
  
  // Menu management
  void add_menu_item(const std::string &title, const std::string &type,
                     const std::string &mqtt_topic, const std::string &mqtt_payload,
                     bool readonly, const std::string &condition_topic,
                     const std::string &condition_value, const std::string &file_path,
                     int max_entries);
  
  void add_submenu_item(const std::string &parent_title, const std::string &title,
                        const std::string &type, const std::string &mqtt_topic,
                        const std::string &mqtt_payload, bool readonly,
                        const std::string &condition_topic, const std::string &condition_value,
                        const std::string &file_path, int max_entries);
  
  // Input handling
  void handle_key_press(uint16_t key, uint8_t modifiers);
  
  // MQTT handling
  void on_mqtt_message(const std::string &topic, const std::string &payload);
  void publish_mqtt_message(const std::string &topic, const std::string &payload);
  
  // Display rendering - now renders directly to Arduino_GFX
  void render_display(bool full_redraw = true);

 private:
  void initialize_display();
  void initialize_usb_keyboard();
  void initialize_buttons();
  
  // USB signal detection for ESP32-8048S070N/C
  void check_usb_signals();
  
  // Button handling
  void check_buttons();
  void handle_button_press(int button_id);
  
  // OTA Safety state
  bool disabled_for_ota_{false};

 protected:
  // Removed display_ since we use integrated Arduino_GFX
  std::string mqtt_topic_prefix_;
  bool boot_sequence_;
  bool cursor_blink_;
  uint32_t font_color_;
  uint32_t background_color_;
  
  // USB keyboard pins (hardcoded for ESP32-8048S070N/C)
  GPIOPin *usb_dp_pin_{nullptr};
  GPIOPin *usb_dm_pin_{nullptr};
  
  // Physical button pins
  GPIOPin *down_button_pin_{nullptr};
  GPIOPin *enter_button_pin_{nullptr};
  GPIOPin *back_button_pin_{nullptr};
  
  // Button state tracking
  bool down_button_last_state_{true};   // Pull-up default
  bool enter_button_last_state_{true};  // Pull-up default  
  bool back_button_last_state_{true};   // Pull-up default
  uint32_t last_button_check_{0};
  
  // USB Host state (simplified for keyboard monitoring only)
  bool usb_host_initialized_{false};
  bool keyboard_connected_{false};
  uint32_t last_usb_check_time_{0};
  
  // Arduino_GFX objects - use void pointers to avoid header conflicts
  void *bus_{nullptr};
  void *rgbpanel_{nullptr};
  void *gfx_{nullptr};
  
  // Terminal state
  TerminalState current_state_;
  TerminalState last_rendered_state_;
  std::vector<MenuItem> main_menu_;
  std::vector<MenuItem> *current_menu_;
  int selected_index_;
  int last_rendered_selected_index_;
  std::vector<int> menu_stack_;  // For navigation back
  bool content_changed_;  // Flag to indicate screen needs full redraw
  bool cursor_state_changed_;  // Flag for cursor blink updates
  // OTA operations now handled by OTA Safety component
  
  // Boot sequence
  bool boot_complete_;
  uint32_t boot_start_time_;
  int boot_line_;
  std::vector<std::string> boot_messages_;
  
  // Display management
  std::vector<std::string> screen_buffer_;
  int scroll_offset_;
  bool cursor_visible_;
  uint32_t last_cursor_toggle_;
  
  // Text editor state
  std::string editor_content_;
  int editor_cursor_pos_;
  int editor_line_;
  bool editor_insert_mode_;
  
  // MQTT state tracking
  std::map<std::string, std::string> mqtt_values_;
  
  // Private methods
  void init_boot_sequence();
  void update_boot_sequence();
  void draw_boot_line(int line_index);
  void render_boot_screen();
  void render_main_menu();
  void render_submenu();
  void render_text_editor();
  void render_action_screen();
  
  void navigate_up();
  void navigate_down();
  void navigate_enter();
  void navigate_escape();
  void handle_menu_navigation(uint16_t key, uint8_t modifiers);
  void handle_text_editor_input(uint16_t key, uint8_t modifiers);
  
  void execute_action(const MenuItem &item);
  void enter_text_editor(const MenuItem &item);
  void update_menu_visibility();
  void update_status_values();
  
  // Arduino_GFX text rendering methods
  void draw_text(int x, int y, const std::string &text, uint32_t color = 0);
  void draw_char(int x, int y, char c, uint32_t color = 0);
  void clear_screen();
  void scroll_up();
  void scroll_down();
  
  // Visual effects for authentic CRT terminal feel
  void render_scan_lines();
  
  // Utility methods
  MenuItemType string_to_menu_type(const std::string &type);
  std::string format_menu_item(const MenuItem &item, bool selected);
  bool should_show_item(const MenuItem &item);
  std::vector<MenuItem> *get_current_menu();
  std::vector<std::string> split_string(const std::string &str, char delimiter);
  
  // File operations for text editor
  std::string load_file(const std::string &path);
  void save_file(const std::string &path, const std::string &content);
};

}  // namespace robco_terminal
}  // namespace esphome
