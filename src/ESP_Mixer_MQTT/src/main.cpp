#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <MQTTManager.h>
#include <DeviceFunction.h>
#include "config.h"

// dont forget to make config.h with char* WIFI_SSID & WIFI_PASSWORD
/*
    #ifndef CONFIG_H
    #define CONFIG_H
        // WiFi Credentials - EDIT THESE VALUES
        const char* WIFI_SSID="***";
        const char* WIFI_PASSWORD="***";
    #endif
    
*/

// MQTT instellingen
const char* mqtt_server = "192.168.1.12";
const int mqtt_port = 1883;

DeviceFunction mixer("Mixer", 26,5);        // Motor op pin 26, state done requirement

MQTTManager mqttManager(
    mqtt_server, mqtt_port,
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