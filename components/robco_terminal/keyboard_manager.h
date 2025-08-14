// File: keyboard_manager.h
#ifndef ROBCO_TERMINAL_KEYBOARD_MANAGER_H
#define ROBCO_TERMINAL_KEYBOARD_MANAGER_H

#include <string>
#include <HardwareSerial.h>

namespace esphome {
namespace robco_terminal {

class RobCoTerminal;  // Forward declaration

class KeyboardManager {
public:
  KeyboardManager(RobCoTerminal* parent);
  void setup();
  void loop();

private:
  RobCoTerminal* parent_;
  // Helper to resolve HID key code to readable string
  static std::string hid_keycode_to_string(uint8_t keycode, uint8_t modifiers);
};

}  // namespace robco_terminal
}  // namespace esphome

#endif