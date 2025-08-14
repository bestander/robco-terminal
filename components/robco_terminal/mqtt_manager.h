// File: mqtt_manager.h
#ifndef ROBCO_TERMINAL_MQTT_MANAGER_H
#define ROBCO_TERMINAL_MQTT_MANAGER_H

#include <string>

namespace esphome {
namespace robco_terminal {

class MqttManager {
public:
  MqttManager(const std::string& topic_prefix);
  void publish(const std::string& topic, const std::string& payload);
  // Note: on_message is called externally, perhaps from ESPHome MQTT component
  void on_message(const std::string& topic, const std::string& payload);

private:
  std::string topic_prefix_;
};

}  // namespace robco_terminal
}  // namespace esphome

#endif