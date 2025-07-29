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

// Helper function to convert RGB888 to RGB565
static uint16_t rgb888_to_rgb565(uint32_t rgb888) {
  uint8_t r = (rgb888 >> 16) & 0xFF;
  uint8_t g = (rgb888 >> 8) & 0xFF;
  uint8_t b = rgb888 & 0xFF;
  
  // Convert to RGB565: 5 bits red, 6 bits green, 5 bits blue
  uint16_t r565 = (r >> 3) & 0x1F;
  uint16_t g565 = (g >> 2) & 0x3F;
  uint16_t b565 = (b >> 3) & 0x1F;
  
  return (r565 << 11) | (g565 << 5) | b565;
}

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
  this->last_rendered_state_ = TerminalState::BOOTING;
  this->boot_complete_ = false;
  this->boot_start_time_ = millis();
  this->boot_line_ = 0;
  this->selected_index_ = 0;
  this->last_rendered_selected_index_ = -1;  // Force initial render
  this->scroll_offset_ = 0;
  this->cursor_visible_ = true;
  this->last_cursor_toggle_ = millis();
  this->current_menu_ = &this->main_menu_;
  this->content_changed_ = true;  // Force initial render
  this->cursor_state_changed_ = false;
  
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

  // Define the RGB panel - optimized timing for stability
  auto *rgbpanel = new Arduino_ESP32RGBPanel(
      41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
      14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
      9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
      15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */,
      0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
      0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
      1 /* pclk_active_neg */, 12000000 /* prefer_speed - reduced from 16MHz to 12MHz for stability */
  );

  // Store the bus and panel
  this->bus_ = bus;
  this->rgbpanel_ = rgbpanel;

  // Define the display with the ST7701 driver - match Arduino version timing
  auto *display = new Arduino_RGB_Display(
      800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, false /* auto_flush - manual control like Arduino */,
      bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
  );
    
  // Store the display
  this->gfx_ = display;

  // Begin the display with exact Arduino timing
  display->begin();
  delay(100); // Longer stabilization delay
  display->fillScreen(BLACK);  
  delay(50); // Let clear settle
  
  #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    delay(20); 
  #endif

  ESP_LOGCONFIG(TAG, "Arduino_GFX display initialized with optimized timing!");
}

void RobCoTerminal::loop() {
  static bool display_initialized = false;
  static uint32_t last_render = 0;
  static bool first_render_done = false;
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
  
  // Force initial render
  if (!first_render_done && display_initialized) {
    ESP_LOGI(TAG, "Forcing initial render");
    // For boot sequence, just clear screen once and let individual lines be drawn
    if (this->current_state_ == TerminalState::BOOTING) {
      auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
      display->fillScreen(BLACK);
      display->flush();
    } else {
      this->render_display(true);
    }
    first_render_done = true;
    last_render = now;
    return;
  }
  
  // Handle cursor blinking - much slower for better stability
  if (this->cursor_blink_ && (now - this->last_cursor_toggle_) > 1500) { // 1.5 second blink rate
    this->cursor_visible_ = !this->cursor_visible_;
    this->last_cursor_toggle_ = now;
    // Only mark cursor state changed, don't force full redraw
    this->cursor_state_changed_ = true;
  }
  
  // Handle boot sequence
  if (!this->boot_complete_ && this->current_state_ == TerminalState::BOOTING) {
    this->update_boot_sequence();
  }
  
  // Check for state changes that require full redraw
  if (this->current_state_ != this->last_rendered_state_) {
    this->content_changed_ = true;
    this->last_rendered_state_ = this->current_state_;
  }
  
  if (this->selected_index_ != this->last_rendered_selected_index_) {
    this->content_changed_ = true;
    this->last_rendered_selected_index_ = this->selected_index_;
  }
  
  // Removed problematic update functions that cause excessive redraws
  // this->update_menu_visibility();
  // this->update_status_values();
  
  // Render display with improved logic to prevent flicker
  // Only render when actually needed and limit frequency
  bool should_render = false;
  bool needs_full_redraw = false;
  
  if (this->content_changed_) {
    // Content changes render immediately but only when necessary
    // Skip full redraws during boot sequence (lines are drawn individually)
    if (this->current_state_ != TerminalState::BOOTING) {
      should_render = true;
      needs_full_redraw = true;
    }
    this->content_changed_ = false;
  } else if (this->cursor_state_changed_ && this->cursor_blink_ && (now - last_render) > 1000) {
    // Cursor updates limited to 1 FPS and no full redraw
    should_render = true;
    needs_full_redraw = false;
    this->cursor_state_changed_ = false;
  }
  
  // Render if needed and not too frequently (min 50ms between renders)
  if (should_render && (now - last_render >= 50)) {
    this->render_display(needs_full_redraw);
    last_render = now;
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
  ESP_LOGI(TAG, "Boot sequence initialized with %d messages", this->boot_messages_.size());
}

void RobCoTerminal::update_boot_sequence() {
  uint32_t elapsed = millis() - this->boot_start_time_;
  
  // Show one line every 300ms (slower for better stability)
  if (elapsed > (this->boot_line_ * 300) && this->boot_line_ < this->boot_messages_.size()) {
    ESP_LOGI(TAG, "Boot line %d: %s", this->boot_line_, 
             this->boot_line_ < this->boot_messages_.size() ? this->boot_messages_[this->boot_line_].c_str() : "");
    
    // Draw just the new line without clearing screen
    this->draw_boot_line(this->boot_line_);
    this->boot_line_++;
    
    // Don't mark content_changed to avoid full screen refresh
    // this->content_changed_ = true;
  }
  
  // Boot complete after all messages shown + 3 seconds (longer delay)
  if (elapsed > (this->boot_messages_.size() * 300 + 3000)) {
    ESP_LOGI(TAG, "Boot sequence complete, switching to main menu");
    this->boot_complete_ = true;
    this->current_state_ = TerminalState::MAIN_MENU;
    this->clear_screen();
    this->content_changed_ = true;  // Mark for redraw only when switching to menu
  }
}

void RobCoTerminal::render_display(bool full_redraw) {
  if (!this->gfx_) return; // Display not initialized yet
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Only clear screen for major state changes, not for cursor updates
  if (full_redraw) {
    display->fillScreen(BLACK);
  }
  
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
  
  // Always flush to ensure content appears on screen
  display->flush();
}

void RobCoTerminal::render_boot_screen() {
  int y = 30; // Start lower to accommodate larger font
  for (int i = 0; i < this->boot_line_ && i < this->boot_messages_.size(); i++) {
    this->draw_text(20, y, this->boot_messages_[i], this->font_color_);
    y += 20; // Larger line spacing for bigger font
  }
  
  // Show cursor on last line if waiting for input
  if (this->boot_line_ >= this->boot_messages_.size() && this->cursor_visible_) {
    int cursor_x = 20 + (this->boot_messages_.back().length() * 12); // Approximate char width for 2x font
    this->draw_char(cursor_x, y - 20, '_', this->font_color_);
  }
}

void RobCoTerminal::draw_boot_line(int line_index) {
  if (line_index >= this->boot_messages_.size()) return;
  
  // Calculate Y position for this line
  int y = 30 + (line_index * 20);
  
  // Draw just this line
  this->draw_text(20, y, this->boot_messages_[line_index], this->font_color_);
  
  // Manually flush just this update to display
  if (this->gfx_) {
    auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
    display->flush();
  }
}

void RobCoTerminal::render_main_menu() {
  // Header with classic RobCo styling - adjusted for larger font
  this->draw_text(20, 30, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->draw_text(20, 60, "ENTER PASSWORD NOW", this->font_color_);
  this->draw_text(20, 90, "> WELCOME, OVERSEER", this->font_color_);
  this->draw_text(20, 120, std::string(50, '='), this->font_color_); // Shorter separator line for larger font
  
  // Menu items with more spacing
  int y = 160;
  for (size_t i = 0; i < this->main_menu_.size(); i++) {
    if (this->should_show_item(this->main_menu_[i])) {
      std::string line = this->format_menu_item(this->main_menu_[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0x80FF80 : this->font_color_; // Brighter version of green
      this->draw_text(40, y, line, color);
      y += 25; // Larger spacing for bigger font
    }
  }
  
  // Status line at bottom
  this->draw_text(20, 440, "Use ARROW KEYS to navigate, ENTER to select", this->font_color_);
}

void RobCoTerminal::render_submenu() {
  if (this->menu_stack_.empty()) return;
  
  int parent_index = this->menu_stack_.back();
  const MenuItem &parent = this->main_menu_[parent_index];
  
  // Header with larger font spacing
  this->draw_text(20, 30, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->draw_text(20, 60, parent.title, this->font_color_);
  this->draw_text(20, 90, std::string(parent.title.length(), '='), this->font_color_);
  
  // Submenu items with proper spacing
  int y = 130;
  for (size_t i = 0; i < parent.subitems.size(); i++) {
    if (this->should_show_item(parent.subitems[i])) {
      std::string line = this->format_menu_item(parent.subitems[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0x80FF80 : this->font_color_;
      this->draw_text(40, y, line, color);
      y += 25; // Larger spacing for bigger font
    }
  }
  
  // Status line
  this->draw_text(20, 440, "Use ARROW KEYS to navigate, ENTER to select", this->font_color_);
}

void RobCoTerminal::render_text_editor() {
  // Header with larger font
  this->draw_text(20, 30, "TERMINAL LOG EDITOR", this->font_color_);
  this->draw_text(20, 60, std::string(20, '='), this->font_color_);
  
  // Content area with adjusted spacing
  int y = 100;
  std::vector<std::string> lines = this->split_string(this->editor_content_, '\n');
  
  for (size_t i = this->scroll_offset_; i < lines.size() && y < 380; i++) {
    this->draw_text(20, y, lines[i], this->font_color_);
    y += 20; // Larger line spacing
  }
  
  // Cursor
  if (this->cursor_visible_) {
    // Simplified cursor position - adjusted for larger font
    int cursor_line = this->editor_cursor_pos_ / 60; // Fewer chars per line due to larger font
    int cursor_col = this->editor_cursor_pos_ % 60;
    int cursor_x = 20 + (cursor_col * 12); // Approximate char width for 2x font
    int cursor_y = 100 + ((cursor_line - this->scroll_offset_) * 20);
    
    if (cursor_y >= 100 && cursor_y < 380) {
      this->draw_char(cursor_x, cursor_y, '_', this->font_color_);
    }
  }
  
  // Status line
  this->draw_text(20, 440, "ESC to save and exit, CTRL+S to save", this->font_color_);
}

void RobCoTerminal::render_action_screen() {
  // Header with larger font and better positioning
  this->draw_text(20, 200, "EXECUTING ACTION...", this->font_color_);
  this->draw_text(20, 240, "Please wait...", this->font_color_);
}

void RobCoTerminal::draw_text(int x, int y, const std::string &text, uint32_t color) {
  if (!this->gfx_ || text.empty()) return;
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Use default green color if not specified (Fallout terminal style)
  if (color == 0) color = 0x00FF00; // Bright green
  
  // Convert RGB888 to RGB565 for Arduino_GFX
  uint16_t rgb565_color = rgb888_to_rgb565(color);
  
  // Set cursor and color
  display->setCursor(x, y);
  display->setTextColor(rgb565_color);
  
  // Use larger text size for authentic terminal feel (2x size)
  display->setTextSize(2, 2); 
  
  // Print the text
  display->print(text.c_str());
  
  ESP_LOGD(TAG, "Drawing text at (%d,%d) with RGB565 color 0x%04X (from 0x%06X): %s", x, y, rgb565_color, color, text.c_str());
}

void RobCoTerminal::draw_char(int x, int y, char c, uint32_t color) {
  if (!this->gfx_) return;
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Use default green color if not specified
  if (color == 0) color = 0x00FF00; // Bright green
  
  // Convert RGB888 to RGB565 for Arduino_GFX
  uint16_t rgb565_color = rgb888_to_rgb565(color);
  
  // Set cursor and color
  display->setCursor(x, y);
  display->setTextColor(rgb565_color);
  
  // Use larger text size to match draw_text (2x size)
  display->setTextSize(2, 2);
  
  // Print the character
  display->print(c);
  
  ESP_LOGD(TAG, "Drawing char at (%d,%d) with RGB565 color 0x%04X (from 0x%06X): %c", x, y, rgb565_color, color, c);
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
  
  this->content_changed_ = true;  // Mark for redraw
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
  
  this->content_changed_ = true;  // Mark for redraw
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
      this->content_changed_ = true;  // Mark for redraw
      break;
    case MenuItemType::ACTION:
      this->execute_action(item);
      break;
    case MenuItemType::TEXT_EDITOR:
      this->enter_text_editor(item);
      this->content_changed_ = true;  // Mark for redraw
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
    this->content_changed_ = true;  // Mark for redraw
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

void RobCoTerminal::render_scan_lines() {
  if (!this->gfx_) return;
  
  // Cast back to Arduino_RGB_Display*
  auto *display = static_cast<Arduino_RGB_Display*>(this->gfx_);
  
  // Draw lighter scan lines for authentic CRT effect
  // Use a very dark green for subtle scan lines
  uint16_t scan_line_color = rgb888_to_rgb565(0x001100); // Very dark green
  
  // Draw horizontal scan lines every 4 pixels for subtle effect (less intensive)
  for (int y = 2; y < 480; y += 4) {
    display->drawFastHLine(0, y, 800, scan_line_color);
  }
  
  // Reduce interference lines frequency to prevent timing issues
  static uint32_t last_interference = 0;
  uint32_t now = millis();
  
  // Add random vertical interference lines less frequently for authentic feel
  if ((now - last_interference) > 10000) { // Every 10 seconds instead of 5
    for (int i = 0; i < 2; i++) { // Fewer lines
      int x = random(800);
      uint16_t interference_color = rgb888_to_rgb565(0x002200); // Slightly brighter dark green
      display->drawFastVLine(x, 0, 480, interference_color);
    }
    last_interference = now;
  }
}

}  // namespace robco_terminal
}  // namespace esphome
