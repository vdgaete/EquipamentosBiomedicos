#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <ArduinoJson.h>
#include <TaskScheduler.h>
#include <Ticker.h>

Ticker timer1;
volatile int flag_send;

#define MSG_BUFFER_SIZE	100
#define REPORTING_PERIOD 5000

uint32_t tsLastReport = 0;
const char* ssid = "MERCUSYS";
const char* password = "ic123456";
const char* mqtt_server = "192.168.1.101";
const int mqtt_port = 1883;

DynamicJsonDocument doc(255);
char buff[250];

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

int value = 0;
int count = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void onBeatDetected() {
    count++;
    Serial.println("Beat");
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

PulseOximeter sensor;

void ICACHE_RAM_ATTR onTime() {
	sensor.update();
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  // Config BioHub
  if (!sensor.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
  sensor.setIRLedCurrent(MAX30100_LED_CURR_11MA);
  sensor.setOnBeatDetectedCallback(onBeatDetected);
  // End Config Bio
  timer1_attachInterrupt(onTime); // Add ISR Function
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(500000);
}

void loop() {
  yield();
  if((millis()-tsLastReport)>REPORTING_PERIOD){ 
    if (count>3){
    if (!client.connected()) {
      reconnect();
    }
    doc["heartrate"] = sensor.getHeartRate();
    doc["oxygen"] = sensor.getSpO2();
    serializeJson(doc,buff);
    Serial.println(buff);
    client.publish("Device/BO",buff);
    client.loop();
    count=0;
    }
  }
}

