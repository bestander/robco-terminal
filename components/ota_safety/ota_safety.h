#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "ESPAsyncWebServer.h"

namespace esphome {
namespace ota_safety {

static const char *const TAG = "ota_safety";

// Forward declaration
class OTASafety;

class OTASafetyWebHandler : public AsyncWebHandler {
 public:
  explicit OTASafetyWebHandler(OTASafety *parent) : parent_(parent) {}
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  OTASafety *parent_;
};

// Simple OTA Safety - just disable RobCo Terminal for next boot only
class OTASafety : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Configuration
  void set_enable_http_endpoints(bool enable) { enable_http_endpoints_ = enable; }

  // Simple API - trigger disable for next boot only
  void disable_for_next_boot();
  bool should_disable_robco_terminal();

 protected:
  void register_http_endpoints();

 private:
  bool enable_http_endpoints_{true};
  bool http_endpoints_registered_{false};
  
  // Persistent storage for one-time disable flag
  ESPPreferenceObject pref_;
  
  OTASafetyWebHandler *web_handler_{nullptr};
};

// Global access function
OTASafety *get_global_ota_safety();

}  // namespace ota_safety
}  // namespace esphome
