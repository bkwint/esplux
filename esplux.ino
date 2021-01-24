#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// if DEBUG_MODE is enabled serial output is enabled as well
#define DEBUG_SERIAL_ENABLED false
#define DEBUG_SERIAL if (DEBUG_SERIAL_ENABLED)Serial

// WIFI configuration, wifi SSID and the wifi password
#define WIFI_HOST "ESPLux"
#define WIFI_SSID ""
#define WIFI_PASS ""

// MQTT configuration
#define MQTT_TOPIC "esplux/livingroom"

// Modify the below broker ip address to the ip address you like to use
IPAddress broker(192,168,1,250);
WiFiClient wclient;

PubSubClient client(wclient);

const int jsonCapacity = JSON_OBJECT_SIZE(50);

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(0x39, 12345);

void setup() {
  DEBUG_SERIAL.begin(9600);

  // setup wifi
  WiFi.hostname(WIFI_HOST);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  DEBUG_SERIAL.print("Connecting to wifi");
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_SERIAL.print(".");
  }

  DEBUG_SERIAL.println();
  DEBUG_SERIAL.print("IP: ");
  DEBUG_SERIAL.println(WiFi.localIP());

  // setup the mqtt sender
  client.setServer(broker, 1883);
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  DEBUG_SERIAL.println(clientId);
  while (!client.connected()) {
    if (client.connect(clientId.c_str())) {
      DEBUG_SERIAL.println("Connected to mqtt");
    }
  }

  // setup the lux sensor
  if (tsl.begin()) {
    DEBUG_SERIAL.println("Sensor found");

    // We always asume no gain to make it work as best as possible
    tsl.setGain(TSL2561_GAIN_1X);
    // fast but bad resolution
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  }
  else {
    DEBUG_SERIAL.println("Sensor not found");
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
      DEBUG_SERIAL.print(event.light); DEBUG_SERIAL.println(" lux");

      char output[128];
      StaticJsonDocument<jsonCapacity> doc;
      doc["lux"] = event.light;

      serializeJson(doc, output);
      if (client.publish(MQTT_TOPIC, output)) {
        DEBUG_SERIAL.println("Published");
        DEBUG_SERIAL.println(output);
      }
    }
  }

  // the disconnect is really import, this will wait for the mqtt 
  // message to have been send, if this step is skipped the esp
  // goes to sleep to quickly for the tcp package to have been send
  client.disconnect();

  #if DEBUG_SERIAL_ENABLED
  // sleep for 5 seconds if we work in debug mode
  ESP.deepSleep(5e6);
  #else
  // sleep for 5 minutes
  ESP.deepSleep(300e6);
  #endif
}
