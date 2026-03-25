# ESP Wake-On-LAN

This project turns an ESP microcontroller into a Wake-On-LAN server. ESP8266 or ESP32 can be used, depending on `platformio.ini`.

A `settings.txt` file must be created inside LittleFS to configure the WiFi settings and MAC address for the Wake-On-LAN device. See [this file](https://github.com/jonafanho/ESP-Wake-On-LAN/blob/master/esp32/data/settings_example.txt) for an example.
