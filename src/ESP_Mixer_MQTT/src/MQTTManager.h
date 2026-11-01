#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DeviceFunction.h"

// MQTT Topic definitions - matching PSOC6 pattern
#define MQTT_CMD_TOPIC                "pancake/mixer/cmd"
#define MQTT_STATE_TOPIC              "pancake/mixer/state"
#define MQTT_ERROR_TOPIC              "pancake/mixer/error"
#define MQTT_STATUS_TOPIC             "pancake/mixer/status"

class MQTTManager {
private:
    // MQTT settings
    const char* mqtt_server;
    const int mqtt_port;
    
    // WiFi credentials
    const char* ssid;
    const char* password;
    
    // State variables
    int stateDone;
    bool mixerDone;
    String currentStatus;
    String clientId;
    
    // Timing
    unsigned long lastUpdate;
    const unsigned long updateInterval;
    
    // Clients
    WiFiClient espClient;
    PubSubClient client;

    // Device
    DeviceFunction* device;
    
    // Private methods
    void setup_wifi();
    void callback(char* topic, byte* payload, unsigned int length);
    void publishJSON(const char* topic, const char* state, const char* message = nullptr, bool isError = false);
    void parseCommand(const char* payload, size_t length);
    void executeMixer();
    
public:
    MQTTManager(const char* server, int port, 
                const char* wifiSSID, const char* wifiPassword,
                DeviceFunction& device);
    ~MQTTManager() = default;
    
    void begin();
    void loop();
    void reconnect();
    void simulateProcess();
    
    // Getters en setters
    int getStateDone() { return stateDone; };
    void setStateDone(int state) { if(state >= 0 && state <= 10) stateDone = state; };
    String getCurrentStatus() { return currentStatus; };
    bool isMixerDone() { return mixerDone; };
};

#endif