#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "IoT";
const char* password = "InternetOfThings";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Simple topics
const char* topic_done = "mydata/done/state";         // Will send 0 <-> x
const char* topic_done_descr = "mydata/done/description"; // Will send description string
const char* topic_status = "mydata/mixer/status";     // Will send "waiting", "mixing", "done"

// Simple state variables
int stateDone = 6;          // Start at part 6
bool mixerDone = false;     // Track if mixer is done
String currentStatus = "waiting";

// Timing
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 3000; // Update every 3 seconds

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.begin(115200); 
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    
    // Create random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      
      // Send initial status with retain flag so new subscribers get it
      client.publish(topic_status, "connected", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishAllData() {
  // Convert stateDone to String for publishing
  char stateStr[10];
  itoa(stateDone, stateStr, 10);
  
  // Publish all current values
  client.publish(topic_done, stateStr, true);                    // "6" or "7"
  client.publish(topic_done_descr, currentStatus.c_str(), true); // Description
  client.publish(topic_status, currentStatus.c_str(), true);     // Status
  
  Serial.println("Published - State: " + String(stateDone) + ", Status: " + currentStatus);
}

void executeMixer() {
    Serial.println("Part 6 completed! Starting mixer: DC Motor");
    // write DC-motor HIGH
    // delay for how long mixing
    delay(2000);
    // write DC-motor pin LOW
    mixerDone = true;
}

void simulateProcess() {
  // Simple simulation - changes every few seconds
  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    
    // State machine logic
    if (stateDone == 6 && !mixerDone) {
      // Part 6 completed, move to mixing
      stateDone = 6;
      currentStatus = "mixing";
      publishAllData();
      executeMixer();
    } 
    else if (mixerDone) {
      // Mixer completed
      mixerDone = true;
      stateDone = 7;
      currentStatus = "mixer_done";
      Serial.println("Mixer completed!");
      publishAllData();

      // Reset cycle
      mixerDone = false;
      stateDone = 6;
      currentStatus = "waiting";
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  
  // Wait for WiFi and MQTT to connect before publishing
  delay(1000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  simulateProcess();
  
  delay(10);
}