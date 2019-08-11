# DF0EG-Wetterstation
Wetterstation der Amateurfunk-Clubstation DF0EG in Burghausen

- ESP8266 Lolin NodeMCU 1.0 / Arduino

- PIN D7:    DSM501a Feinstaubsensor
- PIN D1/D2: I2C BMP85 Luftdruck- / Temperatursensor und SHT21 Luftfeuchte- / Temperatursensor
neu 
- PIN D1/D2: I2C BME280 Luftdruck-, Temperatur- und Luftfeuchtesensor

Die Daten werden via Wifi an einen MQTT Broker und ins APRS-Netz gesendet.

Eine Datei "password.h" muss zusätzlich im Ordner liegen. In dieser müssen folgende Parameter definiert sein:

<pre>
const char mqtt_server[]  = "XXXX.XX";
const int  mqtt_port      = XXXX;
const char mqtt_name[]    = "XXXX";
const char mqtt_user[]    = "XXXX";
const char mqtt_pass[]    = "XXXX";

const char aprs_login[]   = "user XXXXX-XX pass XXXXX";
</pre>

Bibliotheken (über Arduino IDE, außer SHT21):
- ESP8266WiFi.h
- WiFiUdp.h
- ESP8266mDNS.h
- DNSServer.h
- ESP8266WebServer.h
- WiFiManager.h
- ESP8266WiFiMulti.h
- ESP8266HTTPClient.h
- NTPClient.h
- TimeLib.h
- Wire.h
- PubSubClient.h

- Adafruit_BMP085.h
- SHT21.h (https://github.com/markbeee/SHT21)
bzw.
- Adafruit_Sensor.h
- Adafruit_BME280.h
