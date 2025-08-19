# Fallout RobCo Terminal Project

A fully functional Fallout-style terminal interface for ESP32-S3 with 7" display, featuring authentic RobCo Industries theming, MQTT integration with Home Assistant, and text editor capabilities.

## Hardware Requirements

- ESP32-S3 HMI 8M PSRAM 16M Flash with 7" 800x480 RGB LCD display
- USB keyboard (USB host implemented by Raspberry Pi Pico connected via UART; firmware: https://github.com/bestander/pi-pico-usb-uart-host)
- WiFi network with Home Assistant + MQTT broker

## Hardware Reference

**ESP32-S3 HMI Board (used in this project):**
- [AliExpress product link](https://www.aliexpress.us/item/3256807606021455.html?spm=a2g0o.order_list.order_list_main.11.21ef1802b2SiYk&gatewayAdapt=glo2usa)
- Features: ESP32-S3, 7" 800x480 RGB LCD, 8MB PSRAM, 16MB Flash, touch support, USB host, multiple GPIOs, ideal for HMI and display projects.

## Features

✅ **Authentic Fallout Terminal Experience**
- RobCo Industries boot sequence
- Green terminal text on black background
- Blinking cursor animation
- Terminal-style UI navigation

✅ **Home Assistant Integration**
- MQTT command/control interface
- Real-time status updates
- Conditional menu items based on device states

✅ **Navigation System**
- Multi-level menu navigation
- USB keyboard support (Arrow keys, Enter, Escape)
- Context-sensitive menu items

✅ **Text Editor**
- Built-in log editor
- File-based storage
- Terminal-style editing interface

wifi_password: "your_wifi_password"
api_encryption_key: "32-character-key-generated-by-esphome"
ota_password: "your_ota_password"
mqtt_broker: "192.168.1.100"  # Your Home Assistant IP
mqtt_username: "your_mqtt_username"
mqtt_password: "your_mqtt_password"

### Display Issues
- Check SPI pin connections
- Verify display dimensions in configuration
- Ensure adequate power supply

### Keyboard Not Working
- Verify USB host is enabled
- Check USB cable and connections
- Monitor logs for keyboard detection

### MQTT Connection Issues
- Verify Home Assistant MQTT broker settings
- Check network connectivity
- Validate MQTT credentials

### Debug Logging

Enable verbose logging:

```yaml
logger:
  level: VERBOSE
  logs:
    robco_terminal: VERBOSE
```

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is for educational and personal use. Fallout and RobCo Industries are trademarks of Bethesda Softworks.
