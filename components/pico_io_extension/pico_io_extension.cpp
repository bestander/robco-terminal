
#include "pico_io_extension.h"
#include "esphome/core/log.h"
#include "driver/uart.h"

namespace esphome {
namespace pico_io_extension {

static const char *TAG = "pico_io_extension";

void PicoIOExtension::set_uart_pins(int rx, int tx) {
  rx_pin_ = rx;
  tx_pin_ = tx;
}

void PicoIOExtension::set_key_press_callback(std::function<void(uint8_t keycode, uint8_t modifiers)> cb) {
  key_press_cb_ = cb;
}

void PicoIOExtension::setPin(uint8_t pin, bool state) {
  uint8_t cmd[4] = {0x01, pin, static_cast<uint8_t>(state ? 1 : 0), '\n'};
  uart_write_bytes(UART_NUM_1, (const char*)cmd, 4);
}

void PicoIOExtension::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UART (ESP-IDF) on RX=%d, TX=%d, baud rate=9600...", rx_pin_, tx_pin_);
  const uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity    = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
  };
  uart_driver_install(UART_NUM_1, 256, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, tx_pin_, rx_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void PicoIOExtension::loop() {
  uint8_t report[8];
  while (uart_read_bytes(UART_NUM_1, report, 8, 20 / portTICK_PERIOD_MS) == 8) {
    uint8_t modifiers = report[0];
    for (int i = 2; i < 8; ++i) {
      if (report[i] == 0) continue;
      if (key_press_cb_) key_press_cb_(report[i], modifiers);
    }
  }
}

}  // namespace pico_io_extension
}  // namespace esphome
