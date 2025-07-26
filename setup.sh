#!/bin/bash

# RobCo Terminal Setup Script
# This script helps set up the ESPHome environment and configuration

set -e

echo "🤖 RobCo Terminal Setup Script"
echo "=============================="
echo

# Check if ESPHome is installed
if ! command -v esphome &> /dev/null; then
    echo "❌ ESPHome not found. Installing in a virtual environment..."

    # Create venv if it doesn't exist
    if [ ! -d ".venv" ]; then
        python3 -m venv .venv
    fi

    # Activate venv and install esphome
    source .venv/bin/activate
    pip install --upgrade pip
    pip install esphome
    echo "✅ ESPHome installed in .venv"
else
    echo "✅ ESPHome found: $(esphome version)"
fi

# Always activate venv if it exists and not already active
if [ -d ".venv" ] && [ -z "$VIRTUAL_ENV" ]; then
    echo "🔄 Activating Python virtual environment (.venv)..."
    source .venv/bin/activate
fi

# Check if secrets.yaml exists
if [ ! -f "secrets.yaml" ]; then
    echo "📝 Creating secrets.yaml from template..."
    cp secrets.yaml.template secrets.yaml
    echo "⚠️  Please edit secrets.yaml with your actual credentials!"
    echo "   Required: WiFi SSID/password, MQTT broker details"
    echo
else
    echo "✅ secrets.yaml already exists"
fi

# Generate API encryption key if needed
echo "🔑 Checking API encryption key..."
API_KEY_LINE=$(grep -m1 "^api_encryption_key:" secrets.yaml || true)
API_KEY_VALUE=$(echo "$API_KEY_LINE" | awk -F': ' '{print $2}' | tr -d '"')

# Function to check if the key is valid base64 and 32 bytes
is_valid_api_key() {
    if [ -z "$1" ]; then
        return 1
    fi
    # Decode and check length
    decoded=$(echo "$1" | base64 -d 2>/dev/null | wc -c | tr -d ' ')
    if [ "$decoded" -eq 32 ]; then
        return 0
    else
        return 1
    fi
}

if ! is_valid_api_key "$API_KEY_VALUE"; then
    echo "Generating new valid API encryption key (base64, 32 bytes)..."
    API_KEY=$(openssl rand -base64 32)
    # Replace or add the API key in secrets.yaml
    if grep -q "^api_encryption_key:" secrets.yaml; then
        sed -i.bak "s|^api_encryption_key:.*|api_encryption_key: \"$API_KEY\"|" secrets.yaml
    else
        echo "api_encryption_key: \"$API_KEY\"" >> secrets.yaml
    fi
    echo "✅ API encryption key generated and set"
else
    echo "✅ API encryption key already configured and valid"
fi

# Validate configuration
echo "🔍 Validating ESPHome configuration..."
if esphome config robco_terminal.yaml > /dev/null 2>&1; then
    echo "✅ Configuration is valid"
else
    echo "❌ Configuration validation failed!"
    echo "Run 'esphome config robco_terminal.yaml' to see details"
    exit 1
fi

echo
echo "🎉 Setup complete!"
echo
echo "Next steps:"
echo "1. Edit secrets.yaml with your actual WiFi and MQTT credentials"
echo "2. Connect your ESP32-S3 device via USB"
echo "3. Run: esphome run robco_terminal.yaml"
echo "4. Configure Home Assistant MQTT entities as described in README.md"
echo
echo "For updates after initial flash: esphome upload robco_terminal.yaml --device robco-terminal.local"
echo
echo "Happy hacking, Vault Dweller! 🏠⚡"
