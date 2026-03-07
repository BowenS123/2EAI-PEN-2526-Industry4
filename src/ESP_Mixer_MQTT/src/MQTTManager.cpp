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
      currentStatus("waiting"),
      lastUpdate(0),
      updateInterval(3000),
      client(espClient) {
}

MQTTManager::~MQTTManager() {
    // Cleanup
    if (device != nullptr) {
        delete device;
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Check if message is for done state (+ is correct state number)
  if (String(topic) == topic_done_prev) {
    String stateValue = message;
    int newDoneState = stateValue.toInt();
    
    // Validate the new state
    if (newDoneState == 1) {
        stateDone = newDoneState;
        executeMixer();
    }
  }
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
    // Create random client ID
    clientId = device->getDeviceName();
    clientId += String(random(0xffff), HEX);
    String prevTopic = String(topic_done) + String(device->getStateDone());
    String thisTopic = String(topic_done) + String(device->getStateDone() + 1);
    topic_done_prev = strdup(prevTopic.c_str());
    topic_done_this = strdup(thisTopic.c_str());
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      if (client.subscribe(topic_done_prev)) {
        Serial.println("Subscribed to: " + String(topic_done_prev));
      } else {
        Serial.println("Subscription failed!");
      }
      client.publish(topic_status, "connected", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void MQTTManager::publishAllData() {
  // Publish all current values
  if(stateDone == device->getStateDone()+1){
      client.publish(topic_done_this, "1", true);
      
  }  
  client.publish(topic_done_descr, currentStatus.c_str(), true); // Description
  client.publish(topic_status, currentStatus.c_str(), true);     // Status
  Serial.println("Published - State: " + String(stateDone) + ", Status: " + currentStatus);
}

void MQTTManager::executeMixer() {   
    currentStatus = device->getDeviceName();
    publishAllData();
    
    device->execute();
    
    stateDone = device->getStateDone()+1;
    currentStatus += "_done";
    
    Serial.print("State ");
    Serial.print(stateDone);
    Serial.println(": Mixer voltooid!");

    publishAllData();
    Serial.println("===================\n");
}

void MQTTManager::simulateProcess() {
  // Simple simulation - changes every few seconds
  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    if (stateDone == device->getStateDone() ) {
        currentStatus = "mixing";
        publishAllData();
        executeMixer();
    }
  }
}

// methodes voor extern gebruik
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
    
    // Initial publish
    delay(1000);
}
void MQTTManager::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    simulateProcess();
}