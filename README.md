# Fallout RobCo Terminal Project

A fully functional Fallout-style terminal interface for ESP32-S3 with 7" display, featuring authentic RobCo Industries theming, MQTT integration with Home Assistant, and text editor capabilities.

[![Demo](assets/demo.gif)](assets/demo.gif)

## Hardware Requirements

- ESP32-S3 HMI 8M PSRAM 16M Flash with 7" 800x480 RGB LCD display
- USB keyboard (USB host implemented by Raspberry Pi Pico connected via UART; firmware: https://github.com/bestander/pi-pico-usb-uart-host)
- WiFi network with Home Assistant + MQTT broker

## Hardware Reference

**ESP32-S3 HMI Board (used in this project):**

- [7" 800*480 with ESP32-S3](https://www.aliexpress.us/item/3256807606021455.html)
- USB keyboard [40% mechanical fits the STL](https://www.aliexpress.us/item/3256808357667662.html)
- [Raspberry Pi Pico](https://www.amazon.com/Raspberry-Pi-Pico/dp/B09KVB8LVR) to be used as extension board and a USB HID master
- [Micro USB OTG cable](https://www.amazon.com/dp/B0BX9FSCFH) to connect the keyboard to Pico
- USB-A - USB-C cable to connect the keyboard to the OTG cable
- [Right angle USBC cable](https://www.amazon.com/dp/B0DQS6FTX3) to power and flash the ESP32-S3 board when mounted
- [Frensel lense](https://www.amazon.com/dp/B015NR7XGS) for CRT display affect
- 2 LED and 100-150 Ohm resistors and wire soldered to pins 17 and 21 on Pi Pico for additional signaling

## 3D Print case

See assets/stl for all the 3D printing parts originals in [onshape](https://cad.onshape.com/documents/a596f804bfbdc16828dea787/w/d24c8487b87e16e2d8c3f80b/e/312ff6ea062bc67b4dcde5d5?renderMode=0&uiState=68afb409a4b52d6712443022).

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

## Configuration

```yaml
wifi_password: "your_wifi_password"
api_encryption_key: "32-character-key-generated-by-esphome"
ota_password: "your_ota_password"
mqtt_broker: "192.168.1.100"  # Your Home Assistant IP
mqtt_username: "your_mqtt_username"
mqtt_password: "your_mqtt_password"
```

## Home Assistant: MQTT Configuration

1. **Install the Mosquitto broker add-on** (recommended):
   - Go to **Settings > Add-ons > Add-on Store**.
   - Search for "Mosquitto broker" and install it.
   - Start the add-on.

2. **Enable the MQTT integration:**
   - Go to **Settings > Devices & Services**.
   - Click **Add Integration** and search for "MQTT".
   - Enter your broker details (host, port, username, password) matching your ESPHome YAML configuration.
   - Save and finish setup.

3. **Verify connection:**
   - Go to **Settings > Devices & Services > MQTT**.
   - Click **Configure** and use "Listen to a topic" (enter `#` to see all messages) to verify ESPHome is publishing.

4. **Troubleshooting:**
   - Check the Mosquitto broker logs in **Settings > Add-ons > Mosquitto broker > Log**.
   - Ensure your ESPHome device and Home Assistant are on the same network and credentials match.

For more details, see the [Home Assistant MQTT docs](https://www.home-assistant.io/integrations/mqtt/).

## Home Assistant: Example MQTT Scripts & Automations

### Required MQTT Entities

1. **Script: Publish Door State**

Add this to your `scripts.yaml`:
```yaml
publish_garage_state:
  alias: Publish Garage Door State
  sequence:
    - service: mqtt.publish
      data:
        topic: garage/state
        payload: "{{ state }}"
```

Call this script with `state: opened` or `state: closed` when your automation opens/closes the door.

2. **Script: Listen for Open Command with Password**

Add this to your `automations.yaml`:
```yaml
- alias: Open Garage Door via MQTT Password
  trigger:
    - platform: mqtt
      topic: garage/door/open
  condition:
    - condition: template
      value_template: "{{ trigger.payload == 'YOUR_PASSWORD' }}"
  action:
    - service: script.publish_garage_state
      data:
        state: opened
    # Add your open door action here
```

3. **Script: Listen for Close Command**

Add this to your `automations.yaml`:
```yaml
- alias: Close Garage Door via MQTT
  trigger:
    - platform: mqtt
      topic: garage/door/close
  action:
    - service: script.publish_garage_state
      data:
        state: closed
    # Add your close door action here
```

**Note:**
- Replace `YOUR_PASSWORD` with your actual password.
- Add your actual door open/close service calls in the `action` sections.
- Make sure the `script.publish_garage_state` script is available in Home Assistant.

For more details, see the [Home Assistant MQTT docs](https://www.home-assistant.io/integrations/mqtt/).

## Troubleshooting

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

## ESPHome Compile and Run

To compile the firmware:

```
source .venv/bin/activate && esphome compile robco_terminal.yaml
```

To upload and run the firmware (OTA or USB):

```
source .venv/bin/activate && esphome run robco_terminal.yaml
```

To upload via OTA (WiFi):

```
source .venv/bin/activate && esphome upload robco_terminal.yaml --device robco-terminal.local
```

To view device logs:

```
source .venv/bin/activate && esphome logs robco_terminal.yaml
```

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is for educational and personal use. Fallout and RobCo Industries are trademarks of Bethesda Softworks.

The 3d design is derived from a very talented and inspiring [work by Lewin Day](https://hackaday.com/2024/05/21/home-automation-terminal-has-great-post-apocalyptic-look/).