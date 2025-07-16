<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Monitor Health Checker - ESP-IDF Project

This is an ESP-IDF project for SONOFF MINI that monitors HTTP health checks and controls a relay based on the results.

## Project Structure
- `main/` - Main application code
- `components/` - Custom components
- `sdkconfig` - ESP-IDF configuration

## Hardware Configuration (SONOFF MINI)
- GPIO00: BUTTON (configuration mode trigger)
- GPIO01: TX
- GPIO02: AVAILABLE
- GPIO03: RX
- GPIO04: S2 (external switch input)
- GPIO12: Relay and RED LED
- GPIO13: BLUE LED
- GPIO16: OTA jumper pin
- GND: S1 (external switch input)

## Functionality
1. **Configuration Mode**: Press button for 5s to enter AP mode
2. **REST API**: Configure WiFi and health check parameters
3. **Health Check**: Periodic HTTP monitoring
4. **Relay Control**: Based on health check results (200 OK = ON, error = OFF)

## Development Notes
- Use ESP-IDF v4.4 or higher
- Target platform: ESP8266 (SONOFF MINI)
- Network stack: LwIP
- HTTP client: ESP HTTP Client
- Web server: ESP HTTP Server
