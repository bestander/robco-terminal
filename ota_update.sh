#!/bin/bash

# OTA Update Script
# Triggers OTA safe mode and uploads firmware

echo "🚀 RobCo Terminal OTA Update"
echo "============================"

cd "$(dirname "$0")"

# Configuration
ROBCO_DEVICE="robco-terminal.local"
WEB_USER="admin"

# Read OTA password from secrets.yaml
if [ ! -f "secrets.yaml" ]; then
    echo "❌ secrets.yaml not found in current directory"
    exit 1
fi

WEB_PASS=$(grep "^ota_password:" secrets.yaml | sed 's/ota_password: *"\(.*\)"/\1/')

if [ -z "$WEB_PASS" ]; then
    echo "❌ Could not read ota_password from secrets.yaml"
    echo "💡 Make sure secrets.yaml contains: ota_password: \"your_password_here\""
    exit 1
fi

echo "✅ OTA password loaded from secrets.yaml"

# Function to check if device is online
check_device() {
    echo "🔍 Checking if $ROBCO_DEVICE is online..."
    
    if ping -c 1 -W 1000 "$ROBCO_DEVICE" > /dev/null 2>&1; then
        local ip=$(ping -c 1 "$ROBCO_DEVICE" | grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' | head -1)
        echo "✅ Device found at IP: $ip"
        return 0
    else
        echo "❌ Device not found or offline"
        return 1
    fi
}

# Function to trigger OTA mode
trigger_ota_mode() {
    echo "🛡️ Triggering OTA safe mode..."
    
    # Get the IP address directly from ping to avoid DNS resolution issues
    local device_ip=$(ping -c 1 "$ROBCO_DEVICE" | grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' | head -1)
    
    if [ -z "$device_ip" ]; then
        echo "❌ Could not resolve device IP address"
        return 1
    fi
    
    # Send the request using IP address to avoid DNS timeout - device will reboot immediately
    curl -s --max-time 3 -u "${WEB_USER}:${WEB_PASS}" "http://${device_ip}/ota_mode" > /dev/null 2>&1
    
    # Don't check curl exit code as device reboots immediately
    echo "✅ OTA mode trigger sent!"
    echo "⏳ Device is rebooting with screen disabled..."
    return 0
}

# Function to wait for device to come back online
wait_for_device() {
    echo "⏳ Waiting for device to reboot and come back online..."
    
    # Wait for device to go offline first
    sleep 3
    
    local attempts=0
    local max_attempts=30
    
    while [ $attempts -lt $max_attempts ]; do
        if ping -c 1 -W 1000 "$ROBCO_DEVICE" > /dev/null 2>&1; then
            echo "✅ Device is back online and ready for OTA!"
            return 0
        fi
        
        printf "\r⏳ Waiting for device... (%d/%d)" $((attempts + 1)) $max_attempts
        sleep 2
        ((attempts++))
    done
    
    echo ""
    echo "❌ Device didn't come back online within 60 seconds"
    return 1
}

# Function to run OTA upload
run_ota_upload() {
    echo "📡 Starting OTA upload..."
    echo "🖥️ Screen should be BLACK during upload (safe mode active)"
    echo ""
    
    source .venv/bin/activate && esphome run robco_terminal.yaml --device "$ROBCO_DEVICE"
    local upload_result=$?
    
    if [ $upload_result -eq 0 ]; then
        echo ""
        echo "🎉 OTA upload completed successfully!"
        echo "✅ Device will reboot and screen will automatically re-enable"
        echo "🖥️ RobCo Terminal should appear after reboot"
        return 0
    else
        echo ""
        echo "❌ OTA upload failed with exit code: $upload_result"
        return 1
    fi
}

# Main execution
echo "🎯 Purpose: Safe OTA update for RobCo Terminal"
echo "📱 Hardware: ESP32-8048S070N/C"
echo "🛡️ Safety: Disables heavy components during upload"
echo ""

# Check prerequisites
if [ ! -f "robco_terminal.yaml" ]; then
    echo "❌ robco_terminal.yaml not found in current directory"
    exit 1
fi

if [ ! -d ".venv" ]; then
    echo "❌ Python virtual environment not found"
    echo "💡 Run setup.sh first to create the virtual environment"
    exit 1
fi

# Step 1: Check device connectivity
echo "Step 1: Checking device connectivity..."
if ! check_device; then
    echo "❌ Cannot continue - device is not reachable"
    echo "💡 Make sure the device is powered on and connected to WiFi"
    exit 1
fi

echo ""
echo "Step 2: Triggering OTA safe mode..."
if ! trigger_ota_mode; then
    echo "❌ Failed to trigger OTA mode"
    exit 1
fi

echo ""
echo "Step 3: Waiting for device to reboot..."
if ! wait_for_device; then
    echo "❌ Device not responding after reboot"
    exit 1
fi

echo ""
echo "✅ Device is ready for OTA upload!"
echo "🖥️ Screen should be BLACK (RobCo Terminal disabled)"
echo "🛡️ Safe mode is active"
echo ""
read -p "Proceed with firmware upload? (Y/n): " upload_confirm

if [ "$upload_confirm" = "n" ] || [ "$upload_confirm" = "N" ]; then
    echo "OTA upload cancelled."
    echo "💡 Device will remain in safe mode until next reboot"
    exit 0
fi

echo ""
echo "Step 4: Uploading firmware via OTA..."
if run_ota_upload; then
    echo ""
    echo "🎉 OTA Update Complete!"
    echo ""
    echo "✅ What happened:"
    echo "   • Device switched to OTA safe mode (screen disabled)"
    echo "   • Firmware uploaded successfully without reboot issues"
    echo "   • Device rebooted with new firmware"
    echo "   • Screen automatically re-enabled"
    echo ""
    echo "🖥️ Check the device - RobCo Terminal should be running normally"
else
    echo ""
    echo "❌ OTA Update Failed!"
    echo ""
    echo "🔧 Troubleshooting:"
    echo "   • Check WiFi connection"
    echo "   • Verify device is still reachable"
    echo "   • Try USB upload if OTA continues to fail"
    echo "   • Check ESPHome logs for detailed error info"
    exit 1
fi
