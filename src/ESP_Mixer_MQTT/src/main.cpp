#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "MQTTManager.h"
#include "DeviceFunction.h"
#include "config.h"

DeviceFunction mixer("Mixer", 26, 5);

MQTTManager mqttManager(
    MQTT_SERVER, MQTT_PORT,
    WIFI_SSID, WIFI_PASSWORD,
    mixer
);

void setup() {
    mqttManager.begin();
    Serial.println("Setup voltooid - klaar om te draaien");
}

void loop() {
    mqttManager.loop();
    
    // delay voor stabiliteit
    delay(10);
}