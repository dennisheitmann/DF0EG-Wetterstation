# DF0EG-Wetterstation
Wetterstation der Amateurfunk-Clubstation DF0EG in Burghausen

- ESP8266 Lolin NodeMCU 1.0 / Arduino

- PIN D7:    DSM501a Feinstaubsensor
- PIN D1/D2: I2C BMP85 Luftdruck- / Temperatursensor und SHT21 Luftfeuchte- / Temperatursensor

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
