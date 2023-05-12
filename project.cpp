#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define MQTT_SERVER "MQTTBrokerIP"
#define MQTT_PORT 1883
#define MQTT_TOPIC "car/parking"

#define TRIGGER_PIN 2
#define ECHO_PIN 3

const int MAX_PARKING_SPOTS = 5;  // Maximum number of parking spots
bool parkingStatus[MAX_PARKING_SPOTS];  // Array to store parking spot status

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT Broker...");
    if (mqttClient.connect("CarParkingClient")) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.print("MQTT connection failed. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  setupWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // Initialize parking spot status as vacant
  for (int i = 0; i < MAX_PARKING_SPOTS; i++) {
    parkingStatus[i] = true;
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Trigger ultrasonic sensor
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Measure the echo time of the ultrasonic sensor
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance based on the speed of sound
  float distance = duration * 0.034 / 2;

  // Find the nearest vacant parking spot
  int nearestSpot = -1;
  float minDistance = 99999;
  for (int i = 0; i < MAX_PARKING_SPOTS; i++) {
    if (parkingStatus[i] && distance < minDistance) {
      minDistance = distance;
      nearestSpot = i;
    }
  }

  // If a vacant spot is found, occupy it and update the server
  if (nearestSpot != -1) {
    parkingStatus[nearestSpot] = false;
    String message = String(nearestSpot);
    mqttClient.publish(MQTT_TOPIC, message.c_str());
  }

  delay(1000);
}
