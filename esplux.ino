#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// if DEBUG_MODE is enabled serial output is enabled as well
#define DEBUG_MODE 1

// WIFI configuration
#define WIFI_HOST "ESPLux"
#define WIFI_SSID ""
#define WIFI_PASS ""

// MQTT configuration
#define MQTT_TOPIC "esplux/livingroom"

IPAddress broker(192,168,1,250);
WiFiClient wclient;

PubSubClient client(wclient);

const int jsonCapacity = JSON_OBJECT_SIZE(50);

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(0x39, 12345);

void setup() {
  #ifdef DEBUG_MODE
  // put your setup code here, to run once:
  Serial.begin(9600);
  #endif

  // setup wifi
  WiFi.hostname(WIFI_HOST);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  #ifdef DEBUG_MODE
  Serial.print("Connecting to wifi");
  #endif
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG_MODE
    Serial.print(".");
    #endif
  }

  #ifdef DEBUG_MODE
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  #endif

  // setup the mqtt sender
  client.setServer(broker, 1883);
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  #ifdef DEBUG_MODE
  Serial.println(clientId);
  #endif
  while (!client.connected()) {
    if (client.connect(clientId.c_str())) {
      #ifdef DEBUG_MODE
      Serial.println("Connected to mqtt");
      #endif
    }
  }

  // setup the lux sensor
  if (tsl.begin()) {
    #ifdef DEBUG_MODE
    Serial.println("Sensor found");
    #endif

    // We always asume no gain to make it work as best as possible
    tsl.setGain(TSL2561_GAIN_1X);
    // fast but bad resolution
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  }
  else {
    #ifdef DEBUG_MODE
    Serial.println("Sensor not found");
    #endif
  }
}

void loop() {
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    if (client.connected()) {
      #ifdef DEBUG_MODE
      Serial.print(event.light); Serial.println(" lux");
      #endif
      char output[128];
      StaticJsonDocument<jsonCapacity> doc;
      doc["lux"] = event.light;

      serializeJson(doc, output);
      if (client.publish(MQTT_TOPIC, output)) {
        #ifdef DEBUG_MODE
        Serial.println("Published");
        Serial.println(output);
        #endif
      }
    }
  }

  // the disconnect is really import, this will wait for the mqtt 
  // message to have been send, if this step is skipped the esp
  // goes to sleep to quickly for the tcp package to have been send
  client.disconnect();

  #ifdef DEBUG_MODE
  // sleep for 5 seconds if we work in debug mode
  ESP.deepSleep(5e6);
  #else
  // sleep for 5 minutes
  ESP.deepSleep(300e6);
  #endif
}
