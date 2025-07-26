# Fallout RobCo Terminal Project

A fully functional Fallout-style terminal interface for ESP32-S3 with 7" display, featuring authentic RobCo Industries theming, MQTT integration with Home Assistant, and text editor capabilities.

## Hardware Requirements

- ESP32-S3 HMI 8M PSRAM 16M Flash with 7" 800x480 RGB LCD display
- USB keyboard (connected via USB host)
- WiFi network with Home Assistant + MQTT broker

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

## Project Structure

```
robco-terminal/
├── robco_terminal.yaml          # Main ESPHome configuration
├── secrets.yaml.template        # Configuration template
├── secrets.yaml                 # Your actual secrets (create from template)
├── setup.sh                     # Automated setup script
├── components/
│   └── robco_terminal/          # Custom ESPHome component
│       ├── __init__.py          # Component registration
│       ├── robco_terminal.h     # C++ header
│       └── robco_terminal.cpp   # C++ implementation
├── home_assistant_config.yaml   # Example HA configuration
└── README.md                    # This file
```

## Setup Instructions

### Quick Setup (Recommended)
```bash
# Run the automated setup script
./setup.sh
```
This script will:
- Install ESPHome in a virtual environment
- Create `secrets.yaml` from template
- Generate a valid API encryption key
- Validate the configuration

### Manual Setup

#### 1. Install ESPHome

```bash
# Create virtual environment (recommended)
python3 -m venv .venv
source .venv/bin/activate

# Install ESPHome
pip install esphome

# Or using Docker
docker pull esphome/esphome
```

#### 2. Configure Your Secrets

```bash
# Copy the template
cp secrets.yaml.template secrets.yaml

# Edit with your actual values
nano secrets.yaml
```

Fill in your actual WiFi, MQTT, and API credentials:

```yaml
wifi_ssid: "YourWiFiNetwork"
wifi_password: "your_wifi_password"
api_encryption_key: "32-character-key-generated-by-esphome"
ota_password: "your_ota_password"
mqtt_broker: "192.168.1.100"  # Your Home Assistant IP
mqtt_username: "your_mqtt_username"
mqtt_password: "your_mqtt_password"
```

#### 3. Generate API Key

```bash
# Generate encryption key (32-byte base64)
openssl rand -base64 32
# Copy output to secrets.yaml as api_encryption_key
```

#### 4. Validate Configuration

```bash
# Check configuration
esphome config robco_terminal.yaml
```

#### 5. Compile and Upload

```bash
# Activate virtual environment (if using)
source .venv/bin/activate

# First time - compile and upload via USB
esphome run robco_terminal.yaml

# Subsequent updates can be done OTA
esphome upload robco_terminal.yaml --device robco-terminal.local
```

## Development

### ESPHome Workflow

**Use ESPHome for all building and flashing**:
- ESPHome automatically manages dependencies, build flags, and ESP32 complexities
- No additional build tools needed
- Simple YAML-based configuration

**Workflow:**
1. **Building**: Use ESPHome tasks 
2. **IDE Support**: ESPHome provides C/C++ IntelliSense automatically
3. **Configuration**: Edit `robco_terminal.yaml` only

### VS Code Integration
The project includes VS Code tasks for common operations:
- **ESPHome Compile** - Build the project
- **ESPHome Upload** - Flash to device
- **ESPHome Run** - Compile and upload
- **ESPHome Logs** - Monitor device logs
- **ESPHome Config Validate** - Check configuration

### Development Workflow
1. Edit configuration/code
2. **Ctrl+Shift+P** → "Tasks: Run Task" → "ESPHome Config Validate"
3. **Ctrl+Shift+P** → "Tasks: Run Task" → "ESPHome Compile" 
4. **Ctrl+Shift+P** → "Tasks: Run Task" → "ESPHome Upload"
5. Monitor logs and test functionality

## Configuration

### Menu Structure

The terminal menu is configured in `robco_terminal.yaml`. Example structure:

```yaml
robco_terminal:
  menu_items:
    - title: "VAULT DOOR CONTROL"
      type: submenu
      items:
        - title: "Door Status"
          type: status
          mqtt_topic: "vault/door/status"
          readonly: true
        - title: "Open Door"
          type: action
          mqtt_topic: "vault/door/command"
          mqtt_payload: "OPEN"
          condition_topic: "vault/door/status"
          condition_value: "CLOSED"
```

### Menu Item Types

- **`submenu`**: Contains other menu items
- **`action`**: Sends MQTT command when selected
- **`status`**: Displays current value from MQTT topic
- **`text_editor`**: Opens text editor for log files

### Conditional Items

Items can be shown/hidden based on MQTT values:

```yaml
- title: "Open Door"
  type: action
  mqtt_topic: "vault/door/command"
  mqtt_payload: "OPEN"
  condition_topic: "vault/door/status"
  condition_value: "CLOSED"  # Only show when door is closed
```

## Home Assistant Integration

### MQTT Topics

The terminal publishes/subscribes to these topics:

- `robco_terminal/status` - Terminal status
- `robco_terminal/command` - Commands from HA
- `vault/door/status` - Door status (example)
- `vault/door/command` - Door commands (example)

### Example Home Assistant Configuration

```yaml
# configuration.yaml
mqtt:
  switch:
    - name: "Vault Door"
      command_topic: "vault/door/command"
      state_topic: "vault/door/status"
      payload_on: "OPEN"
      payload_off: "CLOSE"
      state_on: "OPEN"
      state_off: "CLOSED"

  sensor:
    - name: "Terminal Status"
      state_topic: "robco_terminal/status"
```

## USB Keyboard Mapping

| Key | Function |
|-----|----------|
| ↑ ↓ | Navigate menu items |
| Enter | Select item / Execute action |
| Escape | Go back / Exit submenu |
| Ctrl+S | Save (in text editor) |

## Customization

### Adding New Menu Items

1. Edit `robco_terminal.yaml`
2. Add new menu item to `menu_items` section
3. Upload updated configuration

### Changing Colors

```yaml
robco_terminal:
  font_color: 0x00FF00      # Green (default)
  background_color: 0x000000 # Black (default)
```

### Custom Boot Messages

Edit `boot_messages_` in `robco_terminal.cpp` to customize the boot sequence.

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

### Python Environment Issues
- Use virtual environment to avoid `externally-managed-environment` error
- Run `./setup.sh` to automatically set up the environment
- Always activate `.venv` before running ESPHome commands

### C/C++ Include Path Issues (VS Code)
ESPHome automatically configures proper IntelliSense for your custom components.

**If you still see include errors:**
1. **Ctrl+Shift+P** → "C/C++: Reset IntelliSense Database"
2. **Ctrl+Shift+P** → "ESPHome: Clean Build Files"
3. Restart VS Code if needed

**File Structure for IDE:**
- `.vscode/c_cpp_properties.json` - Generated by ESPHome for IntelliSense
- `.esphome/build/` - Actual build files (managed by ESPHome)

## Future Enhancements

- [ ] Custom terminal font for more authentic look
- [ ] Sound effects (beeps, clicks)
- [ ] Animation effects
- [ ] Network diagnostics menu
- [ ] System status displays
- [ ] More text editor features
- [ ] Save/load terminal sessions

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is for educational and personal use. Fallout and RobCo Industries are trademarks of Bethesda Softworks.
