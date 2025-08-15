// File: keyboard_manager.cpp
#include "keyboard_manager.h"
#include "robco_terminal.h"  // For RobCoTerminal
#include "esphome/core/log.h"

namespace esphome {
namespace robco_terminal {

static const char *TAG = "robco_terminal.keyboard";

KeyboardManager::KeyboardManager(RobCoTerminal* parent) : parent_(parent) {}

void KeyboardManager::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UART Logger on RX=GPIO17, TX=GPIO18, baud rate=%d...", 9600);
  Serial1.end();
  Serial1.setRxBufferSize(256);
  Serial1.begin(9600, SERIAL_8N1, 17, 18);
  Serial1.flush();
  if (Serial1) {
    ESP_LOGI(TAG, "UART Logger initialized successfully, Serial1 active");
  } else {
    ESP_LOGE(TAG, "UART Logger failed to initialize Serial1");
  }
}

void KeyboardManager::loop() {
  int available_bytes = Serial1.available();
  if (available_bytes > 0 && available_bytes <= 256) {
    ESP_LOGD(TAG, "UART has %d bytes available", available_bytes);
  } else if (available_bytes > 256) {
    ESP_LOGE(TAG, "Invalid available bytes: %d, possible driver error", available_bytes);
    Serial1.flush();
  } else {
    static uint32_t last_debug = 0;
    if (millis() - last_debug > 1000) {
      ESP_LOGD(TAG, "No UART data available");
      last_debug = millis();
    }
  }

  // Read full HID report from UART (expecting 8 bytes per report)
  while (Serial1.available() >= 8) {
    uint8_t report[8];
    for (int i = 0; i < 8; ++i) {
      report[i] = Serial1.read();
    }
    uint8_t modifiers = report[0];
    for (int i = 2; i < 8; ++i) {
      if (report[i] == 0) continue;
      std::string key_str = hid_keycode_to_string(report[i], modifiers);
      ESP_LOGI(TAG, "Key pressed: %s (code: 0x%02X, modifiers: 0x%02X)", key_str.c_str(), report[i], modifiers);
      parent_->handle_key_press(report[i], modifiers);

      if (report[i] == 0x4) {
        ESP_LOGI(TAG, "Toggling pin 21 on 'a' key press");
        static bool pin21_state = false;
        pin21_state = !pin21_state;
  uint8_t pin21[4] = {0x01, 21, static_cast<uint8_t>(pin21_state ? 1 : 0), '\n'};
        Serial1.write(pin21, 4);
      }

      // // Toggle pin 17 when 's' is pressed (keycode 0x16)
      if (report[i] == 0x16) {
        ESP_LOGI(TAG, "Toggling pin 17 on 's' key press");
        static bool pin17_state = false;
        pin17_state = !pin17_state;
  uint8_t cmd_pin17[4] = {0x01, 17, static_cast<uint8_t>(pin17_state ? 1 : 0), '\n'};
        Serial1.write(cmd_pin17, 4);
      }
    }
  }

  static uint32_t last_check = 0;
  if (millis() - last_check > 5000) {
    ESP_LOGD(TAG, "UART Logger status check: active, Serial1 initialized=%d", Serial1 ? 1 : 0);
    last_check = millis();
  }
}

std::string KeyboardManager::hid_keycode_to_string(uint8_t keycode, uint8_t modifiers) {
  bool shift = (modifiers & 0x22);
  switch (keycode) {
    case 0x04: return shift ? "A" : "a";
    case 0x05: return shift ? "B" : "b";
    case 0x06: return shift ? "C" : "c";
    case 0x07: return shift ? "D" : "d";
    case 0x08: return shift ? "E" : "e";
    case 0x09: return shift ? "F" : "f";
    case 0x0A: return shift ? "G" : "g";
    case 0x0B: return shift ? "H" : "h";
    case 0x0C: return shift ? "I" : "i";
    case 0x0D: return shift ? "J" : "j";
    case 0x0E: return shift ? "K" : "k";
    case 0x0F: return shift ? "L" : "l";
    case 0x10: return shift ? "M" : "m";
    case 0x11: return shift ? "N" : "n";
    case 0x12: return shift ? "O" : "o";
    case 0x13: return shift ? "P" : "p";
    case 0x14: return shift ? "Q" : "q";
    case 0x15: return shift ? "R" : "r";
    case 0x16: return shift ? "S" : "s";
    case 0x17: return shift ? "T" : "t";
    case 0x18: return shift ? "U" : "u";
    case 0x19: return shift ? "V" : "v";
    case 0x1A: return shift ? "W" : "w";
    case 0x1B: return shift ? "X" : "x";
    case 0x1C: return shift ? "Y" : "y";
    case 0x1D: return shift ? "Z" : "z";
    case 0x1E: return shift ? "!" : "1";
    case 0x1F: return shift ? "@" : "2";
    case 0x20: return shift ? "#" : "3";
    case 0x21: return shift ? "$" : "4";
    case 0x22: return shift ? "%" : "5";
    case 0x23: return shift ? "^" : "6";
    case 0x24: return shift ? "&" : "7";
    case 0x25: return shift ? "*" : "8";
    case 0x26: return shift ? "(" : "9";
    case 0x27: return shift ? ")" : "0";
    case 0x28: return "Enter";
    case 0x29: return "Esc";
    case 0x2A: return "Backspace";
    case 0x2B: return "Tab";
    case 0x2C: return "Space";
    case 0x2D: return shift ? "_" : "-";
    case 0x2E: return shift ? "+" : "=";
    case 0x2F: return shift ? "{" : "[";
    case 0x30: return shift ? "}" : "]";
    case 0x31: return shift ? "|" : "\\";
    case 0x32: return shift ? "~" : "#";
    case 0x33: return shift ? ":" : ";";
    case 0x34: return shift ? "\"" : "'";
    case 0x35: return shift ? "~" : "`";
    case 0x36: return shift ? "<" : ",";
    case 0x37: return shift ? ">" : ".";
    case 0x38: return shift ? "?" : "/";
    case 0x39: return "Caps Lock";
    case 0x3A: return "F1";
    case 0x3B: return "F2";
    case 0x3C: return "F3";
    case 0x3D: return "F4";
    case 0x3E: return "F5";
    case 0x3F: return "F6";
    case 0x40: return "F7";
    case 0x41: return "F8";
    case 0x42: return "F9";
    case 0x43: return "F10";
    case 0x44: return "F11";
    case 0x45: return "F12";
    case 0x4F: return "Right Arrow";
    case 0x50: return "Left Arrow";
    case 0x51: return "Down Arrow";
    case 0x52: return "Up Arrow";
    default: {
      char buf[32];
      snprintf(buf, sizeof(buf), "Unknown (0x%02x)", keycode);
      return std::string(buf);
    }
  }
};

}  // namespace robco_terminal
}  // namespace esphome