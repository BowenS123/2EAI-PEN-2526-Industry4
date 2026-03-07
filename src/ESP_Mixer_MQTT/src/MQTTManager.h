#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DeviceFunction.h"

class MQTTManager {
private:
    // MQTT settings
    const char* mqtt_server;
    const int mqtt_port;
    
    // Topics
    const char* topic_done = "mydata/done/state/"; // update this to path add /x to post stateDone+1 and subscribe to stateDone
    const char* topic_done_descr = "mydata/done/description";
    const char* topic_status = "mydata/mixer/status";

    const char* topic_done_prev ; // subscribe to topic_done/stateDone
    const char* topic_done_this ; // update this to path add topic_done/x to post stateDone+1

    // WiFi credentials (moeten worden ingesteld via setter of constructor)
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
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    void callback(char* topic, byte* payload, unsigned int length);
    void publishAllData();
    void executeMixer();
    
public:
    MQTTManager(const char* server, int port, 
                const char* wifiSSID, const char* wifiPassword,
                DeviceFunction& device);
    ~MQTTManager();
    
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