simple python + arduino ide scripts to host your very own web monitoring panel for your device

everything should be customizable

the python script must be on a device you want to monitor, personally i use a systemd service to run it automatically after reboot (in venv)

tested on Debian 13 and ESP32 DEV KIT

LIBRARIES:  
  ARDUINO:  
    WiFi.h  
    WiFiUdp.h  
    WebServer.h  
    ArduinoJson.h  
    iWebSocketsServer.h  
  PYTHON:  
  `python3 -m venv venv && source venv/bin/activate && sudo apt update && sudo apt install -y lm-sensors iputils-ping && pip install psutil requests`
