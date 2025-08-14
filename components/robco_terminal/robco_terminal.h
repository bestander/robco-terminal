// File: robco_terminal.h
#ifndef ROBCO_TERMINAL_H
#define ROBCO_TERMINAL_H

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/color.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"
#include "esphome/components/ota_safety/ota_safety.h"

#include <WiFi.h>
#ifdef USE_ARDUINO_OTA
#include <ArduinoOTA.h>
#endif
#include "esp_task_wdt.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <stack>

namespace esphome {
namespace robco_terminal {

// Forward declarations
class DisplayManager;
class KeyboardManager;
class MqttManager;

enum class TerminalState {
  BOOTING,
  MAIN_MENU,
  SUBMENU,
  TEXT_EDITOR,
  EXECUTING_ACTION
};

enum class MenuItemType {
  SUBMENU,
  ACTION,
  STATUS,
  TEXT_EDITOR
};

struct MenuItem {
  std::string title;
  MenuItemType type;
  std::string mqtt_topic;
  std::string mqtt_payload;
  bool readonly = false;
  std::string condition_topic;
  std::string condition_value;
  std::string file_path;
  int max_entries = 0;
  std::string current_value;
  bool visible = true;
  std::vector<MenuItem> subitems;
};

static const int LINES_PER_SCREEN = 20;  // Example value, adjust as needed

class RobCoTerminal : public Component {
public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_mqtt_topic_prefix(const std::string &prefix) { mqtt_topic_prefix_ = prefix; }
  void set_boot_sequence(bool seq) { boot_sequence_ = seq; }
  void set_cursor_blink(bool blink) { cursor_blink_ = blink; }
  void set_font_color(uint32_t color) { font_color_ = color; }
  void set_background_color(uint32_t color) { background_color_ = color; }

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

  // OTA safety methods
  void disable_for_ota();
  void enable_after_ota();

  // Key press handler (called by KeyboardManager)
  void handle_key_press(uint16_t key, uint8_t modifiers);

protected:
  friend class KeyboardManager;  // Allow access to handle_key_press

  std::string mqtt_topic_prefix_;
  bool boot_sequence_ = true;
  bool cursor_blink_ = true;
  uint32_t font_color_ = 0x00FF00;  // Default green
  uint32_t background_color_ = 0x000000;  // Black

private:
  DisplayManager* display_;
  KeyboardManager* keyboard_;
  MqttManager* mqtt_;

  TerminalState current_state_;
  TerminalState last_rendered_state_;
  bool boot_complete_;
  uint32_t boot_start_time_;
  int boot_line_;
  std::vector<std::string> boot_messages_;
  int selected_index_;
  int last_rendered_selected_index_;
  int scroll_offset_;
  bool cursor_visible_;
  uint32_t last_cursor_toggle_;
  std::vector<MenuItem> main_menu_;
  std::stack<int> menu_stack_;
  std::vector<MenuItem> *current_menu_;
  std::string editor_content_;
  int editor_cursor_pos_;
  bool content_changed_;
  bool cursor_state_changed_;
  bool disabled_for_ota_ = false;
  std::vector<std::string> screen_buffer_;

  void init_boot_sequence();
  void update_boot_sequence();
  void render_display(bool full_redraw);
  void render_boot_screen();
  void draw_boot_line(int line_index);
  void render_main_menu();
  void render_submenu();
  void render_text_editor();
  void render_action_screen();
  void handle_menu_navigation(uint16_t key, uint8_t modifiers);
  void navigate_up();
  void navigate_down();
  void navigate_enter();
  void navigate_escape();
  void handle_text_editor_input(uint16_t key, uint8_t modifiers);
  void execute_action(const MenuItem &item);
  void enter_text_editor(const MenuItem &item);
  void update_menu_visibility();
  void update_status_values();
  bool should_show_item(const MenuItem &item);
  std::string format_menu_item(const MenuItem &item, bool selected);
  std::vector<MenuItem> *get_current_menu();
  std::vector<std::string> split_string(const std::string &str, char delimiter);
  MenuItemType string_to_menu_type(const std::string &type);
};

}  // namespace robco_terminal
}  // namespace esphome

#endif