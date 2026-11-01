#include "MQTTManager.h"

MQTTManager::MQTTManager(const char* server, int port, 
                         const char* wifiSSID, const char* wifiPassword,
                         DeviceFunction& device)
    : mqtt_server(server),
      mqtt_port(port),
      ssid(wifiSSID),
      password(wifiPassword),
      device(&device),
      stateDone(0),
      mixerDone(false),
      currentStatus("idle"),
      lastUpdate(0),
      updateInterval(3000),
      client(espClient) {
}

void MQTTManager::setup_wifi(){
    delay(10);
    Serial.println();
    Serial.print("Verbinden met WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi verbonden");
    Serial.print("IP adres: ");
    Serial.println(WiFi.localIP());
}

void MQTTManager::reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        // Create client ID based on device name
        clientId = device->getDeviceName();
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            
            // Subscribe to command topic
            if (client.subscribe(MQTT_CMD_TOPIC)) {
                Serial.println("Subscribed to: " + String(MQTT_CMD_TOPIC));
            } else {
                Serial.println("Subscription failed!");
            }
            
            // Publish initial connected state
            publishJSON(MQTT_STATUS_TOPIC, "connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void MQTTManager::publishJSON(const char* topic, const char* state, const char* message, bool isError) {
    StaticJsonDocument<256> doc;
    char payload[256];
    
    if (isError) {
        // Error format: {"state":"state_value","error":true/false,"message":"detail"}
        doc["state"] = state;
        doc["error"] = (strcmp(state, "ok") != 0);
        if (message != nullptr) {
            doc["message"] = message;
        }
    } else {
        // State format: {"state":"state_value"}
        doc["state"] = state;
        if (message != nullptr) {
            doc["message"] = message;
        }
    }
    
    size_t len = serializeJson(doc, payload, sizeof(payload));
    
    if (client.publish(topic, payload, len)) {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
    } else {
        Serial.println("Publish failed!");
    }
}

void MQTTManager::parseCommand(const char* payload, size_t length) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.f_str());
        publishJSON(MQTT_ERROR_TOPIC, "parse_error", "Invalid JSON format", true);
        return;
    }
    
    // Extract command from JSON
    const char* cmd = doc["cmd"];
    if (cmd == nullptr) {
        Serial.println("No 'cmd' field in JSON");
        publishJSON(MQTT_ERROR_TOPIC, "invalid_cmd", "Missing cmd field", true);
        return;
    }
    
    Serial.print("Received command: ");
    Serial.println(cmd);
    
    // Handle different commands
    if (strcmp(cmd, "start") == 0) {
        currentStatus = "starting";
        publishJSON(MQTT_STATE_TOPIC, currentStatus.c_str());
        executeMixer();
    } 
    else if (strcmp(cmd, "stop") == 0) {
        currentStatus = "stopped";
        publishJSON(MQTT_STATE_TOPIC, currentStatus.c_str());
    } 
    else if (strcmp(cmd, "status") == 0) {
        publishJSON(MQTT_STATE_TOPIC, currentStatus.c_str());
    } 
    else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
        publishJSON(MQTT_ERROR_TOPIC, "unknown_cmd", cmd, true);
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("]");
    
    // Convert payload to string for parsing
    char payloadBuffer[256];
    if (length >= sizeof(payloadBuffer)) {
        Serial.println("Payload too large!");
        return;
    }
    
    memcpy(payloadBuffer, payload, length);
    payloadBuffer[length] = '\0';
    
    Serial.print("Payload: ");
    Serial.println(payloadBuffer);
    
    // Check if message is for command topic
    if (String(topic) == MQTT_CMD_TOPIC) {
        parseCommand(payloadBuffer, length);
    }
}

void MQTTManager::executeMixer() {   
    Serial.print("Executing mixer: ");
    Serial.println(device->getDeviceName());
    
    currentStatus = "mixing";
    publishJSON(MQTT_STATE_TOPIC, "mixing");
    
    // Execute device
    device->execute();
    
    delay(1000); // Simulate mixing time
    
    // Publish done state
    currentStatus = "done";
    stateDone = device->getStateDone() + 1;
    
    publishJSON(MQTT_STATE_TOPIC, "done");
    Serial.println("Mixer execution completed!");
}

void MQTTManager::simulateProcess() {
    // Simple simulation - you can remove this if not needed
    if (millis() - lastUpdate > updateInterval) {
        lastUpdate = millis();
        // Periodically publish current state
        if (currentStatus != "idle" && currentStatus != "done") {
            publishJSON(MQTT_STATE_TOPIC, currentStatus.c_str());
        }
    }
}

void MQTTManager::begin() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== MQTT Manager Start ===");
    
    setup_wifi();

    client.setServer(mqtt_server, mqtt_port);
    
    // Gebruik lambda voor callback om instance methods te kunnen aanroepen
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    
    delay(1000);
}

void MQTTManager::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    simulateProcess();
}