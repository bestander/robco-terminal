#include "ota_safety.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace ota_safety {

static OTASafety *global_ota_safety = nullptr;

OTASafety *get_global_ota_safety() {
  return global_ota_safety;
}

void OTASafety::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Simple OTA Safety...");
  
  // Register as global instance
  global_ota_safety = this;
  
  // Initialize persistent storage for the disable flag
  uint32_t hash_value = fnv1_hash("ota_disable_robco");
  ESP_LOGI(TAG, "🔑 Creating preference with hash: 0x%08X for key: 'ota_disable_robco'", hash_value);
  this->pref_ = global_preferences->make_preference<bool>(hash_value);
  
  ESP_LOGCONFIG(TAG, "Simple OTA Safety setup complete");
}

void OTASafety::loop() {
  uint32_t now = millis();
  
  // Register HTTP endpoints after web server is ready
  if (this->enable_http_endpoints_ && !this->http_endpoints_registered_ && now > 5000) {
    this->register_http_endpoints();
  }
}

void OTASafety::dump_config() {
  ESP_LOGCONFIG(TAG, "Simple OTA Safety:");
  ESP_LOGCONFIG(TAG, "  HTTP Endpoints: %s", this->enable_http_endpoints_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  RobCo Terminal Disabled: %s", this->should_disable_robco_terminal() ? "YES" : "NO");
}

void OTASafety::disable_for_next_boot() {
  ESP_LOGW(TAG, "🛡️ Disabling RobCo Terminal for next boot (OTA mode)");
  
  // Save disable flag to persistent storage
  bool disable_flag = true;
  
  if (this->pref_.save(&disable_flag)) {
    ESP_LOGW(TAG, "💾 OTA disable flag saved - RobCo Terminal will be disabled on next boot");
    
    // Force NVS commit to flash
    global_preferences->sync();
    ESP_LOGI(TAG, "✅ NVS sync completed");
    
    ESP_LOGW(TAG, "🔄 Restarting device in 2 seconds...");
    
    // Delay to ensure NVS write completes
    delay(2000);
    ESP.restart();
  } else {
    ESP_LOGE(TAG, "❌ Failed to save OTA disable flag");
  }
}

bool OTASafety::should_disable_robco_terminal() {
  bool disable_flag = false;
  
  // Load the flag from persistent storage
  if (this->pref_.load(&disable_flag)) {
    ESP_LOGI(TAG, "📖 OTA disable flag loaded from storage: %s", disable_flag ? "TRUE" : "FALSE");
    
    if (disable_flag) {
      ESP_LOGW(TAG, "🛑 RobCo Terminal should be disabled (OTA mode active for this boot)");
      
      // Clear the flag so it only applies to this boot
      bool clear_flag = false;
      if (this->pref_.save(&clear_flag)) {
        ESP_LOGI(TAG, "🔄 OTA disable flag cleared - will be normal on next boot");
        global_preferences->sync();
      } else {
        ESP_LOGE(TAG, "❌ Failed to clear OTA disable flag!");
      }
      
      return true;
    } else {
      ESP_LOGI(TAG, "✅ OTA disable flag is false - RobCo Terminal will run normally");
      return false;
    }
  } else {
    ESP_LOGI(TAG, "✅ No OTA disable flag found - RobCo Terminal will run normally");
    return false;
  }
}

void OTASafety::register_http_endpoints() {
  if (!this->enable_http_endpoints_) {
    return;
  }
  
  ESP_LOGI(TAG, "Registering Simple OTA Safety HTTP endpoints...");
  
  // Get the global web server base instance
  auto *base = web_server_base::global_web_server_base;
  if (base == nullptr) {
    ESP_LOGE(TAG, "WebServerBase not found - cannot register HTTP endpoints");
    return;
  }
  
  // Create and register our web handler
  this->web_handler_ = new OTASafetyWebHandler(this);
  base->add_handler(this->web_handler_);
  
  this->http_endpoints_registered_ = true;
  
  ESP_LOGI(TAG, "✅ Simple OTA Safety HTTP endpoints registered!");
  ESP_LOGI(TAG, "Available endpoints:");
  ESP_LOGI(TAG, "   GET  /ota_mode  - Disable RobCo Terminal and restart for OTA");
}

// Web handler implementation
bool OTASafetyWebHandler::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() != HTTP_GET) {
    return false;
  }
  
  const std::string url = request->url().c_str();
  return (url == "/ota_mode");
}

void OTASafetyWebHandler::handleRequest(AsyncWebServerRequest *request) {
  const std::string url = request->url().c_str();
  
  ESP_LOGI(TAG, "Handling Simple OTA Safety HTTP request: %s", url.c_str());
  
  if (url == "/ota_mode") {
    ESP_LOGI(TAG, "🛡️ HTTP request: Disable RobCo Terminal for OTA");
    request->send(200, "text/plain", "Disabling RobCo Terminal for OTA. Device restarting...");
    
    // Delay a bit to let the response be sent
    delay(100);
    this->parent_->disable_for_next_boot();
    
  } else {
    ESP_LOGE(TAG, "❌ Unknown endpoint: %s", url.c_str());
    request->send(404, "text/plain", "Not Found");
  }
}

}  // namespace ota_safety
}  // namespace esphome
