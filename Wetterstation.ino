#include <PubSubClient.h>
#include <WiFi.h>
#include <EEPROM.h>

#include <Adafruit_BME280.h>

#include "credentials.h"
#include "datapoint.h"

#define DHT_TYPE TempAndHumidity
#define SEALEVELPRESSURE (1013.25)
#define DATAPOINTS_COUNT 4

Adafruit_BME280 bme;
int delayTime;

  
const char* cfg_wifi_ssid = networkSSID;
const char* cfg_wifi_password = networkPassword;

const char* mqtt_server = "192.168.1.11";
const unsigned int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

String uuid;
String topic;
String helperTopic;

DataPoint datapoints[DATAPOINTS_COUNT];
char pub[10];


void setup() {
  // put your setup code here, to run once:

  Serial.begin(19200);
  delayTime = 1000;
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  WiFi.begin(cfg_wifi_ssid, cfg_wifi_password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to the WiFi Network");
  Serial.println(WiFi.localIP());
  
  client.setServer(mqtt_server, mqtt_port);
  client.connect("ESP32");

  EEPROM.begin(512);
  
  setupUuid();

  topic = "Equinox/" + uuid + "/";
  Serial.println(topic);

  setupDataPoints();
}

void setupDataPoints() {
  datapoints[0].name = "temp1";
  datapoints[0].unit = "°C";
  datapoints[0].valfunc = &getTemp;

  datapoints[1].name = "pressure1";
  datapoints[1].unit = "hPa";
  datapoints[1].valfunc = &getPressure;

  datapoints[2].name = "humidity1";
  datapoints[2].unit = "%";
  datapoints[2].valfunc = &getHumidity;

  datapoints[3].name = "altitude1";
  datapoints[3].unit = "m";
  datapoints[3].valfunc = &getAltitude;
}


float getTemp() {
  return bme.readTemperature();
}


float getPressure() {
  return bme.readPressure() / 100.0F;
}


float getHumidity() {
  return bme.readHumidity();
}


float getAltitude() {
  return bme.readAltitude(SEALEVELPRESSURE);
}


void setupUuid() {
  unsigned char k;
  
  k = EEPROM.read(0);
  Serial.print("Value char: ");
  Serial.println(k, DEC);

  if (k == 255) {
    uuid = generateUuid();
    writeUuid(uuid);
  } else {
    uuid = readUuid();
  }
}


String readUuid() {
  unsigned char k;
  int i;
  char data[40];
  int len = 0;

  Serial.println("Reading uuid from EEPROM...");
    
  while (k != '\0' && len < 39) {
    k = EEPROM.read(len);
    data[len] = k; 
    len++;
  }

  Serial.println("Uuid: ");
  Serial.println(String(data));

  return String(data);
}


String generateUuid() {
  String uuid;
  Serial.println("Generating uuid...");

  // TODO: Generate uuid automatically, with http request or libary
  
  uuid = "0ba0bfd0-4dac-11eb-8404-0800200c9a66";
  
  Serial.println("uuid generated: ");
  Serial.println(uuid);

  return uuid;
  
}

void writeUuid(String uuid) {
  int _size = uuid.length();

  Serial.println("Writing uuid to EEPROM...");

  for (int i = 0; i < _size; i++) {
    EEPROM.write(i, uuid[i]);
  }

  EEPROM.write(_size, '\0');
  EEPROM.commit();

  Serial.println("Uuid has been written on EEPROM");
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Trying to reconnect to the MQTT-Server");

    if (client.connect("ESP32")) {
      Serial.println("connected to the MQTT-Server");
    } else {
      Serial.print("Connection to the MQTT-Server failed, rc= ");
      Serial.println(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(delayTime);

  if (!client.connected()) {
    reconnect();
  }

  for (int i = 0; i < DATAPOINTS_COUNT; i++) {
    datapoints[i].val = datapoints[i].valfunc();
    Serial.println(datapoints[i].name + ":");
    Serial.print(datapoints[i].val);
    Serial.println(datapoints[i].unit);

    helperTopic = topic + datapoints[i].name + "/" + datapoints[i].unit;

    client.publish(helperTopic.c_str(), dtostrf(datapoints[i].val, 3, 2, pub));
    }
  
  delay(5000);
}
