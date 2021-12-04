#define MQTT_MAX_PACKET_SIZE 512
#define ON "ON"
#define OFF "OFF"
#define LOOP_DELAY 500
#define RELAY_PIN D1

#include <config.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// Device settings
String deviceId;
bool switchOn = false;
String stateTopic;
String switchCmdTopic;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void sendMQTTSwitchDiscoveryMsg() {
  String discoveryTopic = "homeassistant/switch/std_switch_" + deviceId + "/config";

  DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
  char buffer[MQTT_MAX_PACKET_SIZE];

  doc["name"] = "Switch " + deviceId;
  doc["unique_id"] = "std_switch_" + deviceId;
  doc["stat_t"] = stateTopic;
  doc["cmd_t"] = switchCmdTopic;
  doc["val_tpl"] = "{{ value_json.switch|default(ON) }}";

  size_t n = serializeJson(doc, buffer);

  if (!client.publish(discoveryTopic.c_str(), buffer, n)) {
    Serial.println("Error publishing state");
  }
}

void sendMqttStatus() {
  DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
  char buffer[MQTT_MAX_PACKET_SIZE];

  doc["switch"] = switchOn ? ON : OFF;

  size_t n = serializeJson(doc, buffer);
  Serial.print("Sending mqtt status: ");
  String message;
  serializeJson(doc, message);
  Serial.println(message);

  bool published = client.publish(stateTopic.c_str(), buffer, n);
}

void mqttCallback(char *topic, byte *payload, unsigned int length){
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++) {
      message = message + (char) payload[i];  // convert *byte to string
  }
  Serial.print(message);
  
  if (switchCmdTopic == topic) {
    if (message == ON) {
      switchOn = true;
    } else {
      switchOn = false;
    }
  } else {
    Serial.println("Invalid topic");
  }
  sendMqttStatus();
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.setTimeout(2000);

  deviceId = String(ESP.getChipId(), HEX);
  stateTopic = "home/std_switch_" + deviceId + "/state";
  switchCmdTopic = "home/std_switch_" + deviceId + "/switch";

  while(!Serial) { }
  Serial.println("start");
  Serial.print("Device id: ");
  Serial.println(deviceId);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.println("Connected to Wi-Fi");

  client.setBufferSize(MQTT_MAX_PACKET_SIZE);
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(mqttCallback);

   Serial.println("Connecting to MQTT");

  while (!client.connected()) {
    Serial.print(".");

    if (client.connect((deviceId + " switch").c_str(), MQTT_USER, MQTT_PASS)) {

      Serial.println("Connected to MQTT");

      sendMQTTSwitchDiscoveryMsg();
      client.subscribe(switchCmdTopic.c_str());
    } else {

      Serial.println("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }
  sendMqttStatus();
}

void loop() {
  client.loop();
  if (switchOn) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
  delay(LOOP_DELAY);
}