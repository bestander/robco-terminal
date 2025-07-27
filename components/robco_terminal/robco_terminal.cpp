#include "robco_terminal.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/color.h"

// Include Arduino_GFX for direct display rendering
#include <Wire.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

namespace esphome {
namespace robco_terminal {

// Terminal character map for authentic look
static const uint8_t TERMINAL_FONT[128][16] = {
    // Basic ASCII characters (simplified for example)
    // This would be a full character set for authentic terminal look
    // For now, we'll use system font and enhance later
};

void RobCoTerminal::setup() {
  ESP_LOGCONFIG(TAG, "RobCo Terminal setup - deferring display initialization to loop()");
  ESP_LOGI(TAG, "RobCo Terminal setup starting");
  
  this->current_state_ = TerminalState::BOOTING;
  this->boot_complete_ = false;
  this->boot_start_time_ = millis();
  this->boot_line_ = 0;
  this->selected_index_ = 0;
  this->scroll_offset_ = 0;
  this->cursor_visible_ = true;
  this->last_cursor_toggle_ = millis();
  this->current_menu_ = &this->main_menu_;
  
  ESP_LOGI(TAG, "RobCo Terminal state initialized");
  
  // Initialize screen buffer
  this->screen_buffer_.resize(TerminalFont::LINES_PER_SCREEN);
  ESP_LOGI(TAG, "Screen buffer initialized with %d lines", TerminalFont::LINES_PER_SCREEN);
  
  if (this->boot_sequence_) {
    ESP_LOGI(TAG, "Initializing boot sequence");
    this->init_boot_sequence();
  } else {
    ESP_LOGI(TAG, "Skipping boot sequence, going directly to main menu");
    this->boot_complete_ = true;
    this->current_state_ = TerminalState::MAIN_MENU;
  }
  
  ESP_LOGCONFIG(TAG, "RobCo Terminal setup complete");
}

void RobCoTerminal::initialize_display() {
  ESP_LOGCONFIG(TAG, "Initializing Arduino_GFX display for RobCo Terminal...");
  
  // Exact copy from HelloWorld.ino
  #define TFT_BL 2
  
  // Define a data bus (e.g., SPI) - exact from HelloWorld.ino
  auto *bus = new Arduino_SWSPI(
      GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */
  );

  // Define the RGB panel - exact from HelloWorld.ino  
  auto *rgbpanel = new Arduino_ESP32RGBPanel(
      41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
      9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
      15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */,
      0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
      0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 16000000 /* prefer_speed */
  );

  // Store the bus and panel
  this->bus_ = bus;
  this->rgbpanel_ = rgbpanel;

  // Define the display with the ST7701 driver - exact from HelloWorld.ino
  auto *display = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
      bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
  );
    
  // Store the display
  this->gfx_ = display;

  // Begin the display - exact from HelloWorld.ino
  display->begin();
  display->fillScreen(BLACK);  // BLACK background for terminal
  
  #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  #endif

  ESP_LOGCONFIG(TAG, "Arduino_GFX display initialized for RobCo Terminal!");
}

void RobCoTerminal::loop() {
  static bool display_initialized = false;
  static uint32_t last_render = 0;
  uint32_t now = millis();
  
  // Initialize display in loop after all ESPHome components are ready
  if (!display_initialized && now > 2000) { // Wait 2 seconds for ESPHome to stabilize
    this->initialize_display();
    display_initialized = true;
    return; // Exit early this iteration
  }
  
  if (!display_initialized) {
    return; // Wait for display initialization
  }
  
  // Handle cursor blinking
  if (this->cursor_blink_ && (now - this->last_cursor_toggle_) > 500) {
    this->cursor_visible_ = !this->cursor_visible_;
    this->last_cursor_toggle_ = now;
  }
  
  // Handle boot sequence
  if (!this->boot_complete_ && this->current_state_ == TerminalState::BOOTING) {
    this->update_boot_sequence();
  }
  
  // Update menu visibility based on MQTT states
  this->update_menu_visibility();
  
  // Update status values
  this->update_status_values();
  
  // Render display only every 100ms (10 FPS) or when cursor blinks
  bool should_render = false;
  if ((now - last_render) > 100) { // 10 FPS refresh rate
    should_render = true;
    last_render = now;
  }
  
  // Force render when cursor state changes (for smooth blinking)
  if (this->cursor_blink_ && (now - this->last_cursor_toggle_) < 50) {
    should_render = true;
  }
  
  if (should_render) {
    this->render_display();
  }
}

void RobCoTerminal::dump_config() {
  ESP_LOGCONFIG(TAG, "RobCo Terminal:");
  ESP_LOGCONFIG(TAG, "  MQTT Topic Prefix: %s", this->mqtt_topic_prefix_.c_str());
  ESP_LOGCONFIG(TAG, "  Boot Sequence: %s", YESNO(this->boot_sequence_));
  ESP_LOGCONFIG(TAG, "  Cursor Blink: %s", YESNO(this->cursor_blink_));
  ESP_LOGCONFIG(TAG, "  Font Color: 0x%06X", this->font_color_);
  ESP_LOGCONFIG(TAG, "  Background Color: 0x%06X", this->background_color_);
  ESP_LOGCONFIG(TAG, "  Menu Items: %d", this->main_menu_.size());
  ESP_LOGCONFIG(TAG, "  Arduino_GFX Display: %s", this->gfx_ ? "YES" : "NO");
  
  // Log menu structure
  for (size_t i = 0; i < this->main_menu_.size(); i++) {
    const auto &item = this->main_menu_[i];
    ESP_LOGCONFIG(TAG, "    Menu[%d]: %s (%s)", i, item.title.c_str(), 
                  item.type == MenuItemType::SUBMENU ? "submenu" : 
                  item.type == MenuItemType::ACTION ? "action" : 
                  item.type == MenuItemType::STATUS ? "status" : "text_editor");
  }
}

void RobCoTerminal::init_boot_sequence() {
  ESP_LOGI(TAG, "Initializing boot sequence messages");
  this->boot_messages_ = {
    "RobCo Industries (TM) Termlink Protocol",
    "Established 2075",
    "",
    "Initializing...",
    "Boot Sequence Started",
    "Loading System Drivers...",
    "Checking Memory Banks...",
    "8MB PSRAM Detected",
    "16MB Flash Storage Available",
    "Network Interface: ONLINE",
    "MQTT Client: CONNECTING...",
    "Home Assistant Integration: READY",
    "",
    "System Status: NOMINAL",
    "Security Level: AUTHORIZED",
    "",
    "Welcome to Vault-Tec Terminal System",
    "Have a Nice Day!",
    "",
    "Press any key to continue..."
  };
  ESP_LOGI(TAG, "Boot sequence initialized with %d messages", this->boot_messages_.size());
}

void RobCoTerminal::update_boot_sequence() {
  uint32_t elapsed = millis() - this->boot_start_time_;
  
  // Show one line every 200ms
  if (elapsed > (this->boot_line_ * 200) && this->boot_line_ < this->boot_messages_.size()) {
    ESP_LOGI(TAG, "Boot line %d: %s", this->boot_line_, 
             this->boot_line_ < this->boot_messages_.size() ? this->boot_messages_[this->boot_line_].c_str() : "");
    this->boot_line_++;
  }
  
  // Boot complete after all messages shown + 2 seconds
  if (elapsed > (this->boot_messages_.size() * 200 + 2000)) {
    ESP_LOGI(TAG, "Boot sequence complete, switching to main menu");
    this->boot_complete_ = true;
    this->current_state_ = TerminalState::MAIN_MENU;
    this->clear_screen();
  }
}

void RobCoTerminal::render_display() {
  if (!this->gfx_) return; // Display not initialized yet
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Clear screen with black background
  display->fillScreen(BLACK);
  
  switch (this->current_state_) {
    case TerminalState::BOOTING:
      this->render_boot_screen();
      break;
    case TerminalState::MAIN_MENU:
      this->render_main_menu();
      break;
    case TerminalState::SUBMENU:
      this->render_submenu();
      break;
    case TerminalState::TEXT_EDITOR:
      this->render_text_editor();
      break;
    case TerminalState::EXECUTING_ACTION:
      this->render_action_screen();
      break;
  }
}

void RobCoTerminal::render_boot_screen() {
  int y = 20;
  for (int i = 0; i < this->boot_line_ && i < this->boot_messages_.size(); i++) {
    this->draw_text(10, y, this->boot_messages_[i], this->font_color_);
    y += TerminalFont::CHAR_HEIGHT + 2; // Small line spacing
  }
  
  // Show cursor on last line if waiting for input
  if (this->boot_line_ >= this->boot_messages_.size() && this->cursor_visible_) {
    int cursor_x = 10 + (this->boot_messages_.back().length() * 8); // Approximate char width
    this->draw_char(cursor_x, y - TerminalFont::CHAR_HEIGHT, '_', this->font_color_);
  }
}

void RobCoTerminal::render_main_menu() {
  // Header with classic RobCo styling
  this->draw_text(10, 20, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->draw_text(10, 50, "ENTER PASSWORD NOW", this->font_color_);
  this->draw_text(10, 80, "> WELCOME, OVERSEER", this->font_color_);
  this->draw_text(10, 100, std::string(60, '='), this->font_color_); // Separator line
  
  // Menu items
  int y = 140;
  for (size_t i = 0; i < this->main_menu_.size(); i++) {
    if (this->should_show_item(this->main_menu_[i])) {
      std::string line = this->format_menu_item(this->main_menu_[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0xFFFF00 : this->font_color_; // Yellow for selected, green for normal
      this->draw_text(30, y, line, color);
      y += TerminalFont::CHAR_HEIGHT + 4;
    }
  }
  
  // Status line at bottom
  this->draw_text(10, 450, "Use ARROW KEYS to navigate, ENTER to select, ESC to go back", this->font_color_);
}

void RobCoTerminal::render_submenu() {
  if (this->menu_stack_.empty()) return;
  
  int parent_index = this->menu_stack_.back();
  const MenuItem &parent = this->main_menu_[parent_index];
  
  // Header
  this->draw_text(10, 20, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->draw_text(10, 50, parent.title, this->font_color_);
  this->draw_text(10, 80, std::string(parent.title.length(), '='), this->font_color_);
  
  // Submenu items
  int y = 120;
  for (size_t i = 0; i < parent.subitems.size(); i++) {
    if (this->should_show_item(parent.subitems[i])) {
      std::string line = this->format_menu_item(parent.subitems[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0xFFFF00 : this->font_color_;
      this->draw_text(30, y, line, color);
      y += TerminalFont::CHAR_HEIGHT + 4;
    }
  }
  
  // Status line
  this->draw_text(10, 450, "Use ARROW KEYS to navigate, ENTER to select, ESC to go back", this->font_color_);
}

void RobCoTerminal::render_text_editor() {
  // Header
  this->draw_text(10, 20, "TERMINAL LOG EDITOR", this->font_color_);
  this->draw_text(10, 50, std::string(20, '='), this->font_color_);
  
  // Content area
  int y = 90;
  std::vector<std::string> lines = this->split_string(this->editor_content_, '\n');
  
  for (size_t i = this->scroll_offset_; i < lines.size() && y < 400; i++) {
    this->draw_text(10, y, lines[i], this->font_color_);
    y += TerminalFont::CHAR_HEIGHT + 2;
  }
  
  // Cursor
  if (this->cursor_visible_) {
    // Simplified cursor position
    int cursor_line = this->editor_cursor_pos_ / 80; // Chars per line
    int cursor_col = this->editor_cursor_pos_ % 80;
    int cursor_x = 10 + (cursor_col * 8); // Approximate char width
    int cursor_y = 90 + ((cursor_line - this->scroll_offset_) * (TerminalFont::CHAR_HEIGHT + 2));
    
    if (cursor_y >= 90 && cursor_y < 400) {
      this->draw_char(cursor_x, cursor_y, '_', this->font_color_);
    }
  }
  
  // Status line
  this->draw_text(10, 450, "ESC to save and exit, CTRL+S to save", this->font_color_);
}

void RobCoTerminal::render_action_screen() {
  // Header
  this->draw_text(10, 200, "EXECUTING ACTION...", this->font_color_);
  this->draw_text(10, 240, "Please wait...", this->font_color_);
}

void RobCoTerminal::draw_text(int x, int y, const std::string &text, uint32_t color) {
  if (!this->gfx_ || text.empty()) return;
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Use default green color if not specified (Fallout terminal style)
  if (color == 0) color = 0x00FF00; // Bright green
  
  // Set cursor and color
  display->setCursor(x, y);
  display->setTextColor(color);
  display->setTextSize(1, 1); // Standard size
  
  // Print the text
  display->print(text.c_str());
  
  ESP_LOGD(TAG, "Drawing text at (%d,%d): %s", x, y, text.c_str());
}

void RobCoTerminal::draw_char(int x, int y, char c, uint32_t color) {
  if (!this->gfx_) return;
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Use default green color if not specified
  if (color == 0) color = 0x00FF00; // Bright green
  
  // Set cursor and color
  display->setCursor(x, y);
  display->setTextColor(color);
  display->setTextSize(1, 1);
  
  // Print the character
  display->print(c);
  
  ESP_LOGD(TAG, "Drawing char at (%d,%d): %c", x, y, c);
}

void RobCoTerminal::handle_key_press(uint16_t key, uint8_t modifiers) {
  ESP_LOGD(TAG, "Key pressed: 0x%04X, modifiers: 0x%02X", key, modifiers);
  
  // Handle boot sequence
  if (this->current_state_ == TerminalState::BOOTING && this->boot_complete_) {
    this->current_state_ = TerminalState::MAIN_MENU;
    this->clear_screen();
    return;
  }
  
  // Handle different states
  switch (this->current_state_) {
    case TerminalState::MAIN_MENU:
    case TerminalState::SUBMENU:
      this->handle_menu_navigation(key, modifiers);
      break;
    case TerminalState::TEXT_EDITOR:
      this->handle_text_editor_input(key, modifiers);
      break;
    default:
      break;
  }
}

void RobCoTerminal::handle_menu_navigation(uint16_t key, uint8_t modifiers) {
  switch (key) {
    case 0x52: // Up arrow
      this->navigate_up();
      break;
    case 0x51: // Down arrow
      this->navigate_down();
      break;
    case 0x28: // Enter
      this->navigate_enter();
      break;
    case 0x29: // Escape
      this->navigate_escape();
      break;
  }
}

void RobCoTerminal::navigate_up() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty()) return;
  
  do {
    this->selected_index_--;
    if (this->selected_index_ < 0) {
      this->selected_index_ = menu->size() - 1;
    }
  } while (!this->should_show_item((*menu)[this->selected_index_]));
}

void RobCoTerminal::navigate_down() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty()) return;
  
  do {
    this->selected_index_++;
    if (this->selected_index_ >= menu->size()) {
      this->selected_index_ = 0;
    }
  } while (!this->should_show_item((*menu)[this->selected_index_]));
}

void RobCoTerminal::navigate_enter() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty() || this->selected_index_ >= menu->size()) return;
  
  const MenuItem &item = (*menu)[this->selected_index_];
  
  switch (item.type) {
    case MenuItemType::SUBMENU:
      this->menu_stack_.push_back(this->selected_index_);
      this->selected_index_ = 0;
      this->current_state_ = TerminalState::SUBMENU;
      break;
    case MenuItemType::ACTION:
      this->execute_action(item);
      break;
    case MenuItemType::TEXT_EDITOR:
      this->enter_text_editor(item);
      break;
    case MenuItemType::STATUS:
      // Status items are read-only, do nothing
      break;
  }
}

void RobCoTerminal::navigate_escape() {
  if (this->current_state_ == TerminalState::SUBMENU && !this->menu_stack_.empty()) {
    this->menu_stack_.pop_back();
    this->selected_index_ = 0;
    
    if (this->menu_stack_.empty()) {
      this->current_state_ = TerminalState::MAIN_MENU;
    }
  }
}

void RobCoTerminal::clear_screen() {
  if (this->gfx_) {
    // Cast back to Arduino_RGB_Display*
    auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
    display->fillScreen(BLACK);
  }
  
  // Clear the screen buffer
  for (auto &line : this->screen_buffer_) {
    line.clear();
  }
}

void RobCoTerminal::handle_text_editor_input(uint16_t key, uint8_t modifiers) {
  // TODO: Implement text editor input handling
  ESP_LOGD(TAG, "Text editor input: key=0x%04X, modifiers=0x%02X", key, modifiers);
}

void RobCoTerminal::execute_action(const MenuItem &item) {
  ESP_LOGD(TAG, "Executing action: %s", item.title.c_str());
  if (!item.mqtt_topic.empty()) {
    this->publish_mqtt_message(item.mqtt_topic, item.mqtt_payload);
  }
}

void RobCoTerminal::enter_text_editor(const MenuItem &item) {
  ESP_LOGD(TAG, "Entering text editor for: %s", item.title.c_str());
  this->current_state_ = TerminalState::TEXT_EDITOR;
  this->editor_content_ = "";
  this->editor_cursor_pos_ = 0;
  // TODO: Load file content if file_path is specified
}

void RobCoTerminal::update_menu_visibility() {
  // TODO: Update menu item visibility based on MQTT states
  for (auto &item : this->main_menu_) {
    item.visible = true; // For now, show all items
  }
}

void RobCoTerminal::update_status_values() {
  // TODO: Update status values from MQTT
}

bool RobCoTerminal::should_show_item(const MenuItem &item) {
  return item.visible;
}

std::string RobCoTerminal::format_menu_item(const MenuItem &item, bool selected) {
  std::string prefix = selected ? "> " : "  ";
  std::string suffix = "";
  
  if (item.type == MenuItemType::STATUS && !item.current_value.empty()) {
    suffix = ": " + item.current_value;
  }
  
  return prefix + item.title + suffix;
}

std::vector<MenuItem> *RobCoTerminal::get_current_menu() {
  if (this->current_state_ == TerminalState::MAIN_MENU) {
    return &this->main_menu_;
  } else if (this->current_state_ == TerminalState::SUBMENU && !this->menu_stack_.empty()) {
    int parent_index = this->menu_stack_.back();
    if (parent_index >= 0 && parent_index < this->main_menu_.size()) {
      return &this->main_menu_[parent_index].subitems;
    }
  }
  return nullptr;
}

std::vector<std::string> RobCoTerminal::split_string(const std::string &str, char delimiter) {
  std::vector<std::string> result;
  std::string current;
  
  for (char c : str) {
    if (c == delimiter) {
      result.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  
  if (!current.empty()) {
    result.push_back(current);
  }
  
  return result;
}

void RobCoTerminal::add_menu_item(const std::string &title, const std::string &type,
                                  const std::string &mqtt_topic, const std::string &mqtt_payload,
                                  bool readonly, const std::string &condition_topic,
                                  const std::string &condition_value, const std::string &file_path,
                                  int max_entries) {
  ESP_LOGI(TAG, "Adding menu item: '%s' (type: %s)", title.c_str(), type.c_str());
  MenuItem item;
  item.title = title;
  item.type = this->string_to_menu_type(type);
  item.mqtt_topic = mqtt_topic;
  item.mqtt_payload = mqtt_payload;
  item.readonly = readonly;
  item.condition_topic = condition_topic;
  item.condition_value = condition_value;
  item.file_path = file_path;
  item.max_entries = max_entries;
  
  this->main_menu_.push_back(item);
  ESP_LOGI(TAG, "Menu item added. Total main menu items: %d", this->main_menu_.size());
}

void RobCoTerminal::add_submenu_item(const std::string &parent_title, const std::string &title,
                                     const std::string &type, const std::string &mqtt_topic,
                                     const std::string &mqtt_payload, bool readonly,
                                     const std::string &condition_topic, const std::string &condition_value,
                                     const std::string &file_path, int max_entries) {
  // Find the parent menu item
  for (auto &parent : this->main_menu_) {
    if (parent.title == parent_title && parent.type == MenuItemType::SUBMENU) {
      MenuItem item;
      item.title = title;
      item.type = this->string_to_menu_type(type);
      item.mqtt_topic = mqtt_topic;
      item.mqtt_payload = mqtt_payload;
      item.readonly = readonly;
      item.condition_topic = condition_topic;
      item.condition_value = condition_value;
      item.file_path = file_path;
      item.max_entries = max_entries;
      
      parent.subitems.push_back(item);
      break;
    }
  }
}

MenuItemType RobCoTerminal::string_to_menu_type(const std::string &type) {
  if (type == "submenu") return MenuItemType::SUBMENU;
  if (type == "action") return MenuItemType::ACTION;
  if (type == "status") return MenuItemType::STATUS;
  if (type == "text_editor") return MenuItemType::TEXT_EDITOR;
  return MenuItemType::ACTION; // default
}

}  // namespace robco_terminal
}  // namespace esphome
