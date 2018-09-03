/* Lolin ESP8266 Board NodeMCU Version 1 
   4M Flash / 1M SPIFFS
   SHT21 und BMP85 auf I2C (SCL = D1, SDA = D2)
   DSM501a auf D7
   Daten via APRS und MQTT
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SHT21.h>

#include <PubSubClient.h>

#include "password.h"

const char mqtt_topic_1[] = "DF0EG/Sensor/PM25";
const char mqtt_topic_2[] = "DF0EG/Sensor/TEMP-SHT";
const char mqtt_topic_3[] = "DF0EG/Sensor/HUMI-SHT";
const char mqtt_topic_4[] = "DF0EG/Sensor/TEMP-BMP";
const char mqtt_topic_5[] = "DF0EG/Sensor/BARO-BMP";
const char mqtt_topic_6[] = "DF0EG/Sensor/RSSI";

/* APRS */
#define TCP_HOST_NAME "euro.aprs2.net"
#define TCP_HOST_PORT (14580)

#define PM25PIN D7 //DSM501A input

/* I2C Drucksensor */
Adafruit_BMP085 bmp;
float temp_bmp = (-127.0);
float baro_bmp = (-127.0);
const int seaLevel = 460;

/* I2C Luftfeuchtesensor */
SHT21 SHT21;
float temp_sht = (-127.0);
float humi_sht = (-127.0);

unsigned int  sampletime        = 30;
unsigned long starttime         = 0;
unsigned long measurementtime   = 0;
unsigned long lowpulseoccupancy = 0;
unsigned long duration          = 0;
unsigned long lowtime           = 0;
float         ratio             = 0;
float         concentration     = 0;
float         weight            = 0;

/* Zeit für APRS */
WiFiUDP ntpUDP;
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 1000 * 1000);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

WiFiClient espClient;
PubSubClient client(mqtt_server, mqtt_port, callback, espClient);

boolean reconnect() {
  // Loop until we're reconnected
  int trycount = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      trycount = 0;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      unsigned long lastMillis = millis();
      while (lastMillis + 3000 > millis()) {
        yield();
      }
      trycount++;
      if (trycount > 600) {
        ESP.reset();
      }
    }
  }
  return client.connected();
}

void setup(void) {
  Serial.begin(9600);

  delay(100);

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);

  /* WiFi start-up */
  Serial.println("Starting WiFi...");
  WiFiManager wifiManager;
  wifiManager.setTimeout(900);
  if (!(wifiManager.autoConnect())) {
    delay(100);
    ESP.restart();
    delay(100);
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  delay(100);

  //Wire.begin(int sda, int scl);
  Wire.begin(D2, D1);
  bmp.begin();
  // SHT21.begin(); nicht fuer SHT21 Bibliothek notwendig (https://github.com/e-radionicacom/SHT21-Arduino-Library)

  /* Zeit holen */
  timeClient.begin();

  pinMode(PM25PIN, INPUT);
  // wait 30s for DSM501 to warm up
  for (int i = 1; i <= 30; i++)
  {
    delay(1000); // 1s
    Serial.print(i);
    Serial.println(" s (wait 30s for DSM501 to warm up)");
  }

  readSensors();

  client.loop();

  Serial.println("Starting Measurement...");
  measurementtime = micros();
}

void sendAPRS() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    if (!client.connect(TCP_HOST_NAME, TCP_HOST_PORT)) {
      Serial.println("Connection failed");
      return;
    }
    Serial.println("Sending APRS Message");
    int temp_bmp_f = (float)temp_bmp * 1.8 + 32;
    int baro_bmp_1 = baro_bmp * 10;
    client.println(aprs_login);
    client.print("DF0EG-13>APRS,TCPIP*:@");
    client.printf("%02d", day());
    client.printf("%02d", hour());
    client.printf("%02d", minute());
    client.print("z4810.63N/01250.28E_.../...g...t");
    client.printf("%03d", temp_bmp_f);
    client.print("r...p...P...h");
    client.printf("%02d", (int)humi_sht);
    client.print("b");
    client.printf("%05d", baro_bmp_1);
    client.print("DO7DHWetterstation");
    client.println();
    Serial.println("Closing connection");
    client.stop();
    delay(250);
  }
}

void getSetTime() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  setTime(timeClient.getEpochTime());
}

void readSensors() {
  temp_bmp = bmp.readTemperature();
  baro_bmp = bmp.readSealevelPressure(seaLevel) * 0.01;
  temp_sht = SHT21.getTemperature();
  humi_sht = SHT21.getHumidity();
}

void printSensors() {
  Serial.print("ratio=");
  Serial.println(ratio);
  Serial.print("concentration=");
  Serial.println(concentration);
  Serial.print("weight=");
  Serial.println(weight);
  Serial.print("temp(sht)=");
  Serial.println(temp_sht);
  Serial.print("humi(sht)=");
  Serial.println(humi_sht);
  Serial.print("temp(bmp)=");
  Serial.println(temp_bmp);
  Serial.print("baro(bmp)=");
  Serial.println(baro_bmp);
  Serial.print("rssi=");
  Serial.println(WiFi.RSSI());
}

void publishSensors() {
  char weight_str[11];
  dtostrf(weight, 6, 1, weight_str);
  if (! client.publish(mqtt_topic_1, weight_str)) {
    Serial.println(F("PM25 publish failed"));
  } else {
    Serial.println(F("PM25 publish OK!"));
  }
  char temp_sht_str[8];
  dtostrf(temp_sht, 6, 1, temp_sht_str);
  if (! client.publish(mqtt_topic_2, temp_sht_str)) {
    Serial.println(F("TEMP publish failed"));
  } else {
    Serial.println(F("TEMP publish OK!"));
  }
  char humi_sht_str[8];
  dtostrf(humi_sht, 6, 1, humi_sht_str);
  if (! client.publish(mqtt_topic_3, humi_sht_str)) {
    Serial.println(F("HUMI publish failed"));
  } else {
    Serial.println(F("HUMI publish OK!"));
  }
  char temp_bmp_str[8];
  dtostrf(temp_bmp, 6, 1, temp_bmp_str);
  if (! client.publish(mqtt_topic_4, temp_bmp_str)) {
    Serial.println(F("TEMP publish failed"));
  } else {
    Serial.println(F("TEMP publish OK!"));
  }
  char baro_bmp_str[8];
  dtostrf(baro_bmp, 6, 1, baro_bmp_str);
  if (! client.publish(mqtt_topic_5, baro_bmp_str)) {
    Serial.println(F("BARO publish failed"));
  } else {
    Serial.println(F("BARO publish OK!"));
  }
  char rssi_str[8];
  dtostrf(WiFi.RSSI(), 6, 1, rssi_str);
  if (! client.publish(mqtt_topic_6, rssi_str)) {
    Serial.println(F("RSSI publish failed"));
  } else {
    Serial.println(F("RSSI publish OK!"));
  }
}

void loop(void) {
  if (! client.connected()) {
    reconnect();
    yield();
    lowpulseoccupancy = 0;
    starttime = 0;
    measurementtime = micros();
  }
  lowtime = pulseIn(PM25PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy + lowtime;
  duration = micros() - measurementtime;
  if (duration > (sampletime * 1000000))
  {
    ratio = (float(lowpulseoccupancy) / float(duration)) * 100.0;
    // using adapted: http://bayen.eecs.berkeley.edu/sites/default/files/conferences/Low-cost_coarse_airborne.pdf
    concentration = 848.0 * ratio + 0.457;
    // using adapted: https://github.com/richardhmm/DIYRepo/blob/master/arduino/libraries/DSM501/DSM501.cpp
    weight = 1.0 * (0.1776 * pow((ratio / 100), 3) - 0.24 * pow((ratio / 100), 2) + 94.003 * (ratio / 100));
    int i = 0;
    temp_sht = -127;
    humi_sht = -127;
    temp_bmp = -127;
    baro_bmp = -127;
    do {
      delay(100);
      readSensors();
      i++;
    } while ((temp_bmp == -127 || baro_bmp == -127 || humi_sht == -127 || humi_sht > 100) && (i < 5));
    Serial.println("values:");
    printSensors();
    Serial.println("mqtt:");
    publishSensors();
    Serial.println("aprs:");
    sendAPRS();
    Serial.println("Everything Done!");
    // reset counters
    lowpulseoccupancy = 0;
    starttime = 0;
    Serial.println("Done");
    client.loop();
    measurementtime = micros();
  }
  client.loop();
}
