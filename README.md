# ESP32_ThingsBoard_Monitoring_Patient_Monitoring

This project uses an ESP32 to monitor:
- Pulse
- Temperature
- Movement

The data is sent to a ThingsBoard dashboard via MQTT and displayed using custom widgets (LED indicators, Boolean switch).

## Features
- Real-time monitoring via OLED
- Buzzer alert on abnormal conditions
- MQTT communication with ThingsBoard
- Custom ThingsBoard widgets for movement and alert

## Hardware
- ESP32
- Pulse Sensor
- DHT11 Temperature Sensor
- PIR Motion Sensor
- OLED Display
- Buzzer
- Breadboard + Jumper Wires

## Setup
1. Upload the `.ino` file using Arduino IDE
2. Configure the MQTT Access Token and Wi-Fi credentials
3. Create a ThingsBoard dashboard using provided JSON
4. Create Entity Alias and bind widgets to keys (`alert`, `movement`, etc.)
