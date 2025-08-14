#include "uart_logger.h"
#include "esphome/core/log.h"
#include <HardwareSerial.h>

namespace esphome {
namespace uart_logger {

static const char *const TAG = "uart_logger";

static volatile bool data_received = false;
static volatile size_t data_len = 0;
static char buffer[128];

UARTLogger::UARTLogger() {
  ESP_LOGI(TAG, "UARTLogger component instantiated");
}

void UARTLogger::setup() {
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

void UARTLogger::loop() {
  // Check available bytes
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

  // Read available UART data
  while (Serial1.available() && data_len < sizeof(buffer) - 1) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (data_len > 0) {
        buffer[data_len] = '\0';
        ESP_LOGD(TAG, "Received %d bytes", data_len);
        data_received = true;
      }
      data_len = 0;
    } else {
      buffer[data_len++] = c;
      ESP_LOGD(TAG, "Read byte %d: 0x%02X ('%c')", data_len - 1, c, isprint(c) ? c : '.');
    }
  }

  // Log complete message
  if (data_received) {
    ESP_LOGI(TAG, "Received from Pico: %s", buffer);
    // Optional: Send ACK back to Pico (uncomment if needed)
    // Serial1.println("ACK");
    // ESP_LOGD(TAG, "Sent to Pico: ACK");
    data_received = false;
  }

  // Periodic status check
  static uint32_t last_check = 0;
  if (millis() - last_check > 5000) {
    ESP_LOGD(TAG, "UART Logger status check: active, Serial1 initialized=%d", Serial1 ? 1 : 0);
    last_check = millis();
  }
}

}  // namespace uart_logger
}  // namespace esphome