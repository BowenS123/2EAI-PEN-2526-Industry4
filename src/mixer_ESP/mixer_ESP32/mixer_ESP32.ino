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

String clientId; // uniek id voor apparaat

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
    clientId = "ESPMixer-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      if (client.subscribe(topic_done)) {
        Serial.println("Subscribed to: " + String(topic_done));
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
// Callback function to handle incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (message.indexOf(clientId) != -1) {
    Serial.println("negeer self-published message");
    return;
  }

  // Check if message is for done state
  if (String(topic) == topic_done) {
    int separatorIndex = message.indexOf('|');
    String stateValue = message;
    if (separatorIndex != -1) {
      stateValue = message.substring(0, separatorIndex);
    }
    int newDoneState = stateValue.toInt();
    
    // Validate the new state (assuming valid range 0-10) + when this controller busy (6)
    if(stateDone != 6){
      if (newDoneState >= 0 && newDoneState <= 10) {
        Serial.print("Update state to ");
        Serial.println(newDoneState);
        
        stateDone = newDoneState;
        publishAllData();
      }
    }
  }
}

void publishAllData() {
  // Convert stateDone to String for publishing
  char stateStr[10];
  itoa(stateDone, stateStr, 10);
  
  // Publish all current values
  // Publish with client ID in the message (as a prefix or separate)
  String messageWithId = String(stateStr) + "|" + clientId;
  client.publish(topic_done, messageWithId.c_str(), true);

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
    stateDone = 7;
    currentStatus = "mixer_done";
    Serial.println("Part 7: Mixer completed!");
    publishAllData();
}

void simulateProcess() {
  // Simple simulation - changes every few seconds
  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    if (stateDone == 6) {
        // Part 6 completed, move to mixing
        currentStatus = "mixing";
        publishAllData();
        executeMixer();
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Set the callback function
  
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