#!/bin/bash

# RobCo Terminal Setup Script
# This script helps set up the ESPHome environment and configuration

set -e

echo "🤖 RobCo Terminal Setup Script"
echo "=============================="
echo

# Check if ESPHome is installed
if ! command -v esphome &> /dev/null; then
    echo "❌ ESPHome not found. Installing..."
    pip3 install esphome
    echo "✅ ESPHome installed"
else
    echo "✅ ESPHome found: $(esphome version)"
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
if ! grep -q "api_encryption_key:" secrets.yaml || grep -q "generate_with_esphome" secrets.yaml; then
    echo "Generating new API encryption key..."
    API_KEY=$(openssl rand -base64 32 | tr -d "=+/" | cut -c1-32)
    
    # Replace or add the API key in secrets.yaml
    if grep -q "api_encryption_key:" secrets.yaml; then
        sed -i.bak "s/api_encryption_key:.*/api_encryption_key: \"$API_KEY\"/" secrets.yaml
    else
        echo "api_encryption_key: \"$API_KEY\"" >> secrets.yaml
    fi
    
    echo "✅ API encryption key generated"
else
    echo "✅ API encryption key already configured"
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
