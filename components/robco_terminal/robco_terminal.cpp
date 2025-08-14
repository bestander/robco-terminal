// File: robco_terminal.cpp
#include "robco_terminal.h"
#include "display_manager.h"
#include "keyboard_manager.h"
#include "mqtt_manager.h"
#include "esphome/core/log.h"

// Terminal character map for authentic look (unused in current code, kept for future)
static const uint8_t TERMINAL_FONT[128][16] = {
    // Basic ASCII characters (simplified for example)
    // This would be a full character set for authentic terminal look
    // For now, we'll use system font and enhance later
};

namespace esphome {
namespace robco_terminal {

static const char *TAG = "robco_terminal";

void RobCoTerminal::setup() {
  ESP_LOGCONFIG(TAG, "RobCo Terminal setup - deferring display initialization to loop()");
  ESP_LOGI(TAG, "RobCo Terminal setup starting");

  auto *ota_safety = ota_safety::get_global_ota_safety();
  if (ota_safety != nullptr) {
    ESP_LOGI(TAG, "OTA Safety system found - checking disable flag...");
  
    // Check if we should be disabled for this boot
    bool should_disable = ota_safety->should_disable_robco_terminal();
    ESP_LOGW(TAG, "OTA Safety check result: should_disable = %s", should_disable ? "TRUE" : "FALSE");
  
    if (should_disable) {
      ESP_LOGW(TAG, "🛑 OTA Safety requires component to be disabled - skipping initialization");
      this->disabled_for_ota_ = true;
      return;
    } else {
      ESP_LOGI(TAG, "✅ OTA Safety allows normal operation - proceeding with setup");
    }
  } else {
    ESP_LOGW(TAG, "⚠️ OTA Safety system not found - running without OTA protection");
  }

  // Configure ArduinoOTA for better stability with large firmware
  #ifdef USE_ARDUINO_OTA
  ESP_LOGI(TAG, "Configuring ArduinoOTA buffer size for large firmware...");
  ArduinoOTA.setUpdateBufferSize(1024);  // Smaller buffer for better stability
  ESP_LOGI(TAG, "ArduinoOTA buffer size set to 1024 bytes");
  #endif

  // Register OTA callbacks for better stability
  ESP_LOGI(TAG, "Registering OTA event callbacks...");

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
  // OTA operations now handled by OTA Safety component

  ESP_LOGI(TAG, "RobCo Terminal state initialized");

  // Initialize screen buffer with memory optimization for OTA
  this->screen_buffer_.resize(LINES_PER_SCREEN);
  ESP_LOGI(TAG, "Screen buffer initialized with %d lines", LINES_PER_SCREEN);

  if (this->boot_sequence_) {
    ESP_LOGI(TAG, "Initializing boot sequence");
    this->init_boot_sequence();
  } else {
    ESP_LOGI(TAG, "Skipping boot sequence, going directly to main menu");
    this->boot_complete_ = true;
    this->current_state_ = TerminalState::MAIN_MENU;
  }

  // Create managers
  display_ = new DisplayManager();
  keyboard_ = new KeyboardManager(this);
  mqtt_ = new MqttManager(this->mqtt_topic_prefix_);

  keyboard_->setup();

  ESP_LOGCONFIG(TAG, "RobCo Terminal setup complete");
}

void RobCoTerminal::loop() {
  // Early exit if disabled for OTA
  if (this->disabled_for_ota_) {
    return;
  }

  static bool display_initialized = false;
  static uint32_t last_render = 0;
  static bool first_render_done = false;
  static uint32_t wifi_yield_counter = 0;
  uint32_t now = millis();

  // OTA detection is now handled by OTA Safety component

  // Initialize display in loop after all ESPHome components are ready
  if (!display_initialized && now > 2000) { // Wait 2 seconds for ESPHome to stabilize
    this->display_->init();
    display_initialized = true;
    return; // Exit early this iteration
  }

  if (!display_initialized) {
    return; // Wait for display initialization
  }

  // WiFi interference mitigation: Yield to WiFi stack every few iterations
  wifi_yield_counter++;
  if (wifi_yield_counter % 10 == 0) {
    yield(); // Give WiFi stack time to process
    delay(1); // Small delay to prevent tight looping
  }

  // Force initial render
  if (!first_render_done && display_initialized) {
    ESP_LOGI(TAG, "Forcing initial render");
    // For boot sequence, just clear screen once and let individual lines be drawn
    if (this->current_state_ == TerminalState::BOOTING) {
      this->display_->clearScreen();
      this->display_->flush();
    } else {
      this->render_display(true);
    }
    first_render_done = true;
    last_render = now;
    return;
  }

  // Handle cursor blinking - much slower for better stability and WiFi coexistence
  if (this->cursor_blink_ && (now - this->last_cursor_toggle_) > 2000) { // 2 second blink rate (slower for WiFi stability)
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

  // Render display with improved logic to prevent flicker and WiFi interference
  // Only render when actually needed and limit frequency more aggressively
  bool should_render = false;
  bool needs_full_redraw = false;
  uint32_t min_render_interval = 100; // Increased from 50ms to 100ms for WiFi coexistence

  // Adaptive rendering based on WiFi activity
  static uint32_t last_wifi_check = 0;
  if ((now - last_wifi_check) > 1000) {
    // Check WiFi status and adjust rendering accordingly
    if (WiFi.status() == WL_CONNECTED) {
      // WiFi is active, reduce rendering frequency more
      min_render_interval = 150;
    } else {
      // No WiFi, can render more frequently
      min_render_interval = 75;
    }
    last_wifi_check = now;
  }

  // Only reduce rendering frequency if we're definitely in OTA mode
  if (this->disabled_for_ota_) {
    min_render_interval = 1000; // Much slower during OTA to avoid interference
    ESP_LOGD(TAG, "Disabled for OTA - reducing render frequency");
  }

  if (this->content_changed_) {
    // Content changes render immediately but only when necessary
    // Skip full redraws during boot sequence (lines are drawn individually)
    if (this->current_state_ != TerminalState::BOOTING) {
      should_render = true;
      needs_full_redraw = true;
    }
    this->content_changed_ = false;
  } else if (this->cursor_state_changed_ && this->cursor_blink_ && (now - last_render) > 1500) {
    // Cursor updates limited to slower rate and no full redraw (WiFi friendly)
    should_render = true;
    needs_full_redraw = false;
    this->cursor_state_changed_ = false;
  }

  // Render if needed and not too frequently (adaptive interval based on WiFi activity)
  if (should_render && (now - last_render >= min_render_interval)) {
    this->render_display(needs_full_redraw);
    last_render = now;
  }

  this->keyboard_->loop();
}

void RobCoTerminal::dump_config() {
  ESP_LOGCONFIG(TAG, "RobCo Terminal:");
  ESP_LOGCONFIG(TAG, "  MQTT Topic Prefix: %s", this->mqtt_topic_prefix_.c_str());
  ESP_LOGCONFIG(TAG, "  Boot Sequence: %s", YESNO(this->boot_sequence_));
  ESP_LOGCONFIG(TAG, "  Cursor Blink: %s", YESNO(this->cursor_blink_));
  ESP_LOGCONFIG(TAG, "  Font Color: 0x%06X", this->font_color_);
  ESP_LOGCONFIG(TAG, "  Background Color: 0x%06X", this->background_color_);
  ESP_LOGCONFIG(TAG, "  Menu Items: %d", this->main_menu_.size());

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
  }

  // Boot complete after all messages shown + 3 seconds (longer delay)
  if (elapsed > (this->boot_messages_.size() * 300 + 3000)) {
    ESP_LOGI(TAG, "Boot sequence complete, switching to main menu");
    this->boot_complete_ = true;
    this->current_state_ = TerminalState::MAIN_MENU;
    this->display_->clearScreen();
    this->content_changed_ = true;  // Mark for redraw only when switching to menu
  }
}

void RobCoTerminal::render_display(bool full_redraw) {
  // Only clear screen for major state changes, not for cursor updates
  if (full_redraw) {
    this->display_->clearScreen();
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
  this->display_->flush();
}

void RobCoTerminal::render_boot_screen() {
  int y = 30; // Start lower to accommodate larger font
  for (int i = 0; i < this->boot_line_ && i < this->boot_messages_.size(); i++) {
    this->display_->drawText(20, y, this->boot_messages_[i], this->font_color_);
    y += 20; // Larger line spacing for bigger font
  }

  // Show cursor on last line if waiting for input
  if (this->boot_line_ >= this->boot_messages_.size() && this->cursor_visible_) {
    int cursor_x = 20 + (this->boot_messages_.back().length() * 12); // Approximate char width for 2x font
    this->display_->drawChar(cursor_x, y - 20, '_', this->font_color_);
  }
}

void RobCoTerminal::draw_boot_line(int line_index) {
  if (line_index >= this->boot_messages_.size()) return;

  // Calculate Y position for this line
  int y = 30 + (line_index * 20);

  // Draw just this line
  this->display_->drawText(20, y, this->boot_messages_[line_index], this->font_color_);

  // Manually flush just this update to display
  this->display_->flush();
}

void RobCoTerminal::render_main_menu() {
  // Header with classic RobCo styling - adjusted for larger font
  this->display_->drawText(20, 30, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->display_->drawText(20, 60, "ENTER PASSWORD NOW", this->font_color_);
  this->display_->drawText(20, 90, "> WELCOME, OVERSEER", this->font_color_);
  this->display_->drawText(20, 120, std::string(50, '='), this->font_color_); // Shorter separator line for larger font

  // Menu items with more spacing
  int y = 160;
  for (size_t i = 0; i < this->main_menu_.size(); i++) {
    if (this->should_show_item(this->main_menu_[i])) {
      std::string line = this->format_menu_item(this->main_menu_[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0x80FF80 : this->font_color_; // Brighter version of green
      this->display_->drawText(40, y, line, color);
      y += 25; // Larger spacing for bigger font
    }
  }

  // Status line at bottom
  this->display_->drawText(20, 440, "Use ARROW KEYS to navigate, ENTER to select", this->font_color_);
}

void RobCoTerminal::render_submenu() {
  if (this->menu_stack_.empty()) return;

  int parent_index = this->menu_stack_.top();
  const MenuItem &parent = this->main_menu_[parent_index];

  // Header with larger font spacing
  this->display_->drawText(20, 30, "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", this->font_color_);
  this->display_->drawText(20, 60, parent.title, this->font_color_);
  this->display_->drawText(20, 90, std::string(parent.title.length(), '='), this->font_color_);

  // Submenu items with proper spacing
  int y = 130;
  for (size_t i = 0; i < parent.subitems.size(); i++) {
    if (this->should_show_item(parent.subitems[i])) {
      std::string line = this->format_menu_item(parent.subitems[i], i == this->selected_index_);
      uint32_t color = (i == this->selected_index_) ? 0x80FF80 : this->font_color_;
      this->display_->drawText(40, y, line, color);
      y += 25; // Larger spacing for bigger font
    }
  }

  // Status line
  this->display_->drawText(20, 440, "Use ARROW KEYS to navigate, ENTER to select", this->font_color_);
}

void RobCoTerminal::render_text_editor() {
  // Header with larger font
  this->display_->drawText(20, 30, "TERMINAL LOG EDITOR", this->font_color_);
  this->display_->drawText(20, 60, std::string(20, '='), this->font_color_);

  // Content area with adjusted spacing
  int y = 100;
  std::vector<std::string> lines = this->split_string(this->editor_content_, '\n');

  for (size_t i = this->scroll_offset_; i < lines.size() && y < 380; i++) {
    this->display_->drawText(20, y, lines[i], this->font_color_);
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
      this->display_->drawChar(cursor_x, cursor_y, '_', this->font_color_);
    }
  }

  // Status line
  this->display_->drawText(20, 440, "ESC to save and exit, CTRL+S to save", this->font_color_);
}

void RobCoTerminal::render_action_screen() {
  // Header with larger font and better positioning
  this->display_->drawText(20, 200, "EXECUTING ACTION...", this->font_color_);
  this->display_->drawText(20, 240, "Please wait...", this->font_color_);
}

void RobCoTerminal::handle_key_press(uint16_t key, uint8_t modifiers) {
  ESP_LOGI(TAG, "=== KEY PRESS EVENT ===");
  ESP_LOGI(TAG, "Key code: 0x%04X (%d)", key, key);
  ESP_LOGI(TAG, "Modifiers: 0x%02X (%d)", modifiers, modifiers);
  ESP_LOGI(TAG, "Current state: %d", (int)this->current_state_);
  ESP_LOGI(TAG, "Boot complete: %s", this->boot_complete_ ? "YES" : "NO");

  // Log special keys first (these are NOT printable ASCII)
  if (key == 0x28) ESP_LOGI(TAG, "Special key: ENTER");
  else if (key == 0x29) ESP_LOGI(TAG, "Special key: ESCAPE");
  else if (key == 0x2A) ESP_LOGI(TAG, "Special key: BACKSPACE");
  else if (key == 0x2B) ESP_LOGI(TAG, "Special key: TAB");
  else if (key == 0x2C) ESP_LOGI(TAG, "Special key: SPACE");
  else if (key == 0x4F) ESP_LOGI(TAG, "Special key: RIGHT ARROW");
  else if (key == 0x50) ESP_LOGI(TAG, "Special key: LEFT ARROW");
  else if (key == 0x51) ESP_LOGI(TAG, "Special key: DOWN ARROW");
  else if (key == 0x52) ESP_LOGI(TAG, "Special key: UP ARROW");
  else if (key >= 32 && key <= 126) {
    // Only log ASCII for actual printable characters (not special keys)
    ESP_LOGI(TAG, "ASCII character: '%c'", (char)key);
  } else {
    ESP_LOGI(TAG, "Non-printable key: 0x%04X", key);
  }

  // Handle boot sequence
  if (this->current_state_ == TerminalState::BOOTING && this->boot_complete_) {
    ESP_LOGI(TAG, "Boot complete - any key pressed, switching to main menu");
    this->current_state_ = TerminalState::MAIN_MENU;
    this->display_->clearScreen();
    this->content_changed_ = true;
    return;
  }

  // Handle different states
  switch (this->current_state_) {
    case TerminalState::MAIN_MENU:
    case TerminalState::SUBMENU:
      ESP_LOGI(TAG, "Handling menu navigation");
      this->handle_menu_navigation(key, modifiers);
      break;
    case TerminalState::TEXT_EDITOR:
      ESP_LOGI(TAG, "Handling text editor input");
      this->handle_text_editor_input(key, modifiers);
      break;
    default:
      ESP_LOGI(TAG, "No handler for current state: %d", (int)this->current_state_);
      break;
  }
  ESP_LOGI(TAG, "=== KEY PRESS HANDLED ===");
}

void RobCoTerminal::handle_menu_navigation(uint16_t key, uint8_t modifiers) {
  ESP_LOGI(TAG, "Menu navigation - Key: 0x%04X, Current selection: %d", key, this->selected_index_);

  switch (key) {
    case 0x52: // Up arrow
      ESP_LOGI(TAG, "UP ARROW pressed - navigating up");
      this->navigate_up();
      break;
    case 0x51: // Down arrow
      ESP_LOGI(TAG, "DOWN ARROW pressed - navigating down");
      this->navigate_down();
      break;
    case 0x28: // Enter
      ESP_LOGI(TAG, "ENTER pressed - selecting item");
      this->navigate_enter();
      break;
    case 0x29: // Escape
      ESP_LOGI(TAG, "ESCAPE pressed - going back");
      this->navigate_escape();
      break;
    default:
      ESP_LOGI(TAG, "Unhandled key in menu navigation: 0x%04X", key);
      break;
  }
}

void RobCoTerminal::navigate_up() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty()) {
    ESP_LOGI(TAG, "Navigate up - no menu or empty menu");
    return;
  }

  int old_index = this->selected_index_;

  do {
    this->selected_index_--;
    if (this->selected_index_ < 0) {
      this->selected_index_ = menu->size() - 1;
    }
  } while (!this->should_show_item((*menu)[this->selected_index_]));

  ESP_LOGI(TAG, "Navigate up - selection changed from %d to %d", old_index, this->selected_index_);
  this->content_changed_ = true;  // Mark for redraw
}

void RobCoTerminal::navigate_down() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty()) {
    ESP_LOGI(TAG, "Navigate down - no menu or empty menu");
    return;
  }

  int old_index = this->selected_index_;

  do {
    this->selected_index_++;
    if (this->selected_index_ >= menu->size()) {
      this->selected_index_ = 0;
    }
  } while (!this->should_show_item((*menu)[this->selected_index_]));

  ESP_LOGI(TAG, "Navigate down - selection changed from %d to %d", old_index, this->selected_index_);
  this->content_changed_ = true;  // Mark for redraw
}

void RobCoTerminal::navigate_enter() {
  std::vector<MenuItem> *menu = this->get_current_menu();
  if (!menu || menu->empty() || this->selected_index_ >= menu->size()) {
    ESP_LOGI(TAG, "Navigate enter - invalid menu state");
    return;
  }

  const MenuItem &item = (*menu)[this->selected_index_];
  ESP_LOGI(TAG, "Navigate enter - selected item: '%s' (type: %d)", item.title.c_str(), (int)item.type);

  switch (item.type) {
    case MenuItemType::SUBMENU:
      ESP_LOGI(TAG, "Entering submenu: %s", item.title.c_str());
      this->menu_stack_.push(this->selected_index_);
      this->selected_index_ = 0;
      this->current_state_ = TerminalState::SUBMENU;
      this->content_changed_ = true;  // Mark for redraw
      break;
    case MenuItemType::ACTION:
      ESP_LOGI(TAG, "Executing action: %s", item.title.c_str());
      this->execute_action(item);
      break;
    case MenuItemType::TEXT_EDITOR:
      ESP_LOGI(TAG, "Entering text editor: %s", item.title.c_str());
      this->enter_text_editor(item);
      this->content_changed_ = true;  // Mark for redraw
      break;
    case MenuItemType::STATUS:
      ESP_LOGI(TAG, "Status item selected (read-only): %s", item.title.c_str());
      // Status items are read-only, do nothing
      break;
  }
}

void RobCoTerminal::navigate_escape() {
  if (this->current_state_ == TerminalState::SUBMENU && !this->menu_stack_.empty()) {
    this->menu_stack_.pop();
    this->selected_index_ = 0;

    if (this->menu_stack_.empty()) {
      this->current_state_ = TerminalState::MAIN_MENU;
    }
    this->content_changed_ = true;  // Mark for redraw
  }
}

void RobCoTerminal::handle_text_editor_input(uint16_t key, uint8_t modifiers) {
  ESP_LOGI(TAG, "Text editor input - Key: 0x%04X, Modifiers: 0x%02X", key, modifiers);

  // Handle special keys
  if (key == 0x29) { // Escape
    ESP_LOGI(TAG, "ESC pressed in text editor - exiting to main menu");
    this->current_state_ = TerminalState::MAIN_MENU;
    this->content_changed_ = true;
    return;
  }

  // Handle printable characters
  if (key >= 32 && key <= 126) {
    ESP_LOGI(TAG, "Adding character '%c' to editor content", (char)key);
    this->editor_content_ += (char)key;
    this->editor_cursor_pos_++;
    this->content_changed_ = true;
  }

  // Handle special editing keys
  if (key == 0x2A) { // Backspace
    if (!this->editor_content_.empty() && this->editor_cursor_pos_ > 0) {
      ESP_LOGI(TAG, "Backspace pressed - removing character");
      this->editor_content_.pop_back();
      this->editor_cursor_pos_--;
      this->content_changed_ = true;
    }
  }

  if (key == 0x28) { // Enter
    ESP_LOGI(TAG, "Enter pressed in editor - adding newline");
    this->editor_content_ += '\n';
    this->editor_cursor_pos_++;
    this->content_changed_ = true;
  }
}

void RobCoTerminal::execute_action(const MenuItem &item) {
  ESP_LOGD(TAG, "Executing action: %s", item.title.c_str());
  if (!item.mqtt_topic.empty()) {
    this->mqtt_->publish(item.mqtt_topic, item.mqtt_payload);
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
    int parent_index = this->menu_stack_.top();
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

void RobCoTerminal::disable_for_ota() {
  ESP_LOGW(TAG, "🛑 RobCo Terminal disabled for OTA safety");
  this->disabled_for_ota_ = true;

  // Clear display if it exists
  this->display_->clearScreen();
  this->display_->drawText(100, 200, "OTA UPDATE IN PROGRESS", 0x07E0);  // Green
  this->display_->drawText(150, 250, "Please wait...", 0x07E0);
  this->display_->flush();

  ESP_LOGW(TAG, "Display cleared and OTA message shown");
}

void RobCoTerminal::enable_after_ota() {
  ESP_LOGI(TAG, "🔄 RobCo Terminal re-enabled after OTA");
  this->disabled_for_ota_ = false;

  // Force a full redraw when re-enabled
  this->content_changed_ = true;

  ESP_LOGI(TAG, "Component re-enabled, will redraw on next loop");
}

}  // namespace robco_terminal
}  // namespace esphome