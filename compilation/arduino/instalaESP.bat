@echo off
arduino-cli config init --overwrite --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json,https://dl.espressif.com/dl/package_esp32_index.json &&arduino-cli core update-index &&arduino-cli core install esp8266:esp8266&&arduino-cli core install esp32:esp32