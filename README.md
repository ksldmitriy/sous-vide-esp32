# DIY Sous Vide on ESP32
This project provides ESP32 firmware for a DIY sous vide cooker with a web interface that allows you to monitor and set the temperature remotely.

## Demo
![Demo Screenshot](https://github.com/ksldmitriy/sous-vide-esp32/blob/master/demo.png)

## Configuration
- Set your WiFi credentials by editing the `WIFI_SSID` and `WIFI_PASSWORD` values.
- Define the GPIO pin connected to the heater by setting `HEATER_PIN`.
- The ESP32 will connect to the specified WiFi network.
- You can assign a reserved IP address for the device in your router's DHCP server settings.
- After that, access the web interface through your browser using that IP address.

## Build Instructions
1. Connect the ESP32 to your computer.
2. Ensure that ESP-IDF is installed on your system.
3. Run `idf configure` to configure the project.
4. Use `build.sh` to build the firmware.
5. Use `run.sh` to flash the firmware onto the ESP32 and start monitoring.
