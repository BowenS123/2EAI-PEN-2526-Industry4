# API & Integration Guide - RobotArm

## Inleiding

Dit document beschrijft de publieke APIs, structuren en functies waarmee je het RobotArm-systeem kunt integreren of uitbreiden.

---

## 1. Publieke Header Files

### 1.1 mqtt_task.h - MQTT Management Interface

```c
/*
 * File: source/mqtt_task.h
 * Doel: Publieke interface voor MQTT client management
 */

#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "cy_mqtt_api.h"

/* Task Parameters */
#define MQTT_CLIENT_TASK_PRIORITY   (2)
#define MQTT_CLIENT_TASK_STACK_SIZE (1024 * 2)

/* Global Event Commands */
typedef enum {
    HANDLE_MQTT_SUBSCRIBE_FAILURE = 0,
    HANDLE_MQTT_PUBLISH_FAILURE = 1,
    HANDLE_DISCONNECTION = 2
} mqtt_task_cmd_t;

/* Global Variables (extern) */
extern cy_mqtt_t mqtt_connection;      // MQTT handle
extern QueueHandle_t mqtt_task_q;      // Event queue
extern uint32_t status_flag;           // Initialization status flags

/* Functions */
void mqtt_client_task(void *pvParameters);  // Main MQTT task

#endif /* MQTT_TASK_H_ */
```

**Status Flags**:
```c
#define WCM_INITIALIZED         (1lu << 0)
#define WIFI_CONNECTED          (1lu << 1)
#define LIBS_INITIALIZED        (1lu << 2)
#define BUFFER_INITIALIZED      (1lu << 3)
#define MQTT_INSTANCE_CREATED   (1lu << 4)
#define MQTT_CONNECTION_SUCCESS (1lu << 5)
```

### 1.2 publisher_task.h - Publishing Interface

```c
#ifndef PUBLISHER_TASK_H_
#define PUBLISHER_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"

/* Task Parameters */
#define PUBLISHER_TASK_PRIORITY    (1)
#define PUBLISHER_TASK_STACK_SIZE  (512)
#define PUBLISHER_TASK_QUEUE_LENGTH (4u)

/* Command Types */
typedef enum {
    PUBLISHER_INIT = 0,
    PUBLISHER_DEINIT = 1,
    PUBLISH_STATE = 2,
    PUBLISH_ERROR = 3
} publisher_cmd_t;

/* Data Structure for Queue */
typedef struct {
    publisher_cmd_t cmd;    // Command type
    const char *state;      // State string (voor PUBLISH_STATE)
    const char *detail;     // Detail/error message (voor PUBLISH_ERROR)
} publisher_data_t;

/* Global Variables (extern) */
extern TaskHandle_t publisher_task_handle;
extern QueueHandle_t publisher_task_q;

/* Functions */
void publisher_task(void *pvParameters);  // Main publisher task

#endif /* PUBLISHER_TASK_H_ */
```

**Voorbeeld Gebruik**:
```c
// In andere tasks: publish state update
publisher_data_t cmd = {
    .cmd = PUBLISH_STATE,
    .state = "moving"
};
xQueueSend(publisher_task_q, &cmd, portMAX_DELAY);
```

### 1.3 subscriber_task.h - Command Reception Interface

```c
#ifndef SUBSCRIBER_TASK_H_
#define SUBSCRIBER_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"

/* Task Parameters */
#define SUBSCRIBER_TASK_PRIORITY    (1)
#define SUBSCRIBER_TASK_STACK_SIZE  (1024)
#define SUBSCRIBER_TASK_QUEUE_LENGTH (2u)

/* Robot Command Types */
typedef enum {
    ROBOT_CMD_UNKNOWN = 0,
    ROBOT_CMD_MOVE = 1,
    ROBOT_CMD_STOP = 2,
    ROBOT_CMD_CLEAR_ERROR = 3,
    ROBOT_CMD_TEACH = 4
} robot_cmd_t;

/* Robot State Enum */
typedef enum {
    ROBOT_STATE_IDLE = 0,
    ROBOT_STATE_MOVING = 1,
    ROBOT_STATE_ERROR = 2,
    ROBOT_STATE_HOMING = 3
} robot_state_t;

/* Device Pose Definitions */
typedef enum {
    DEVICE_POSE_CAMERA = 0,
    DEVICE_POSE_INDUCTIVE = 1,
    DEVICE_POSE_SCALE = 2,
    DEVICE_POSE_MIXER = 3,
    DEVICE_POSE_DISCARD = 4,
    DEVICE_POSE_COUNT = 5
} device_pose_t;

/* Data Structure for Queue */
typedef struct {
    robot_cmd_t cmd;       // Command type
    device_pose_t pose;    // Target pose (voor ROBOT_CMD_MOVE)
} subscriber_data_t;

/* Global Variables (extern) */
extern TaskHandle_t subscriber_task_handle;
extern QueueHandle_t subscriber_task_q;
extern robot_state_t current_state;

/* Functions */
void subscriber_task(void *pvParameters);  // Main subscriber task

#endif /* SUBSCRIBER_TASK_H_ */
```

### 1.4 device_function.h - Hardware Abstraction Layer

```c
#ifndef DEVICE_FUNCTION_H_
#define DEVICE_FUNCTION_H_

#include "cyhal.h"
#include "cybsp.h"
#include <stdbool.h>
#include <string.h>

/* Motor Pin Definitions */
#define STEP1_PIN   P9_0
#define DIR1_PIN    P9_1
#define EN1_PIN     P9_2

#define STEP2_PIN   P10_0
#define DIR2_PIN    P10_1
#define EN2_PIN     P10_2

#define STEP3_PIN   P10_3
#define DIR3_PIN    P10_4
#define EN3_PIN     P10_5

#define SERVO_PIN   P10_6
#define SAVE_BUTTON_PIN P0_4

/* Motor Telemetry */
extern int M1_Pos;      // Motor 1 absolute position (steps)
extern int M2_Pos;      // Motor 2 absolute position (steps)
extern int M3_Pos;      // Motor 3 absolute position (steps)
extern cyhal_pwm_t pwm; // Servo PWM handle

/* Device Function Structure */
typedef struct {
    char name[32];
    int current_step_count;
    int target_step_count;
    bool is_moving;
    device_pose_t teach_motor_index;
    // ... meer fields
} DeviceFunction;

/* Global Variable */
extern DeviceFunction robot_arm;
extern bool robot_arm_initialized;

/* Function Prototypes */
void DeviceFunction_init(DeviceFunction *df, 
                         const char *name,
                         uint32_t gpio_pin,
                         uint32_t priority);

bool DeviceFunction_move(DeviceFunction *df, 
                         device_pose_t target_pose);

bool DeviceFunction_stop(DeviceFunction *df);

bool DeviceFunction_handle_save(DeviceFunction *df);

bool save_pressed(void);  // Check if save button pressed

#endif /* DEVICE_FUNCTION_H_ */
```

---

## 2. Configuration APIs

### 2.1 mqtt_client_config.h - MQTT Settings

```c
/* MQTT Broker Configuration */
#define MQTT_BROKER_ADDRESS         "192.168.1.240"
#define MQTT_PORT                   1883
#define MQTT_SECURE_CONNECTION      0
#define MQTT_USERNAME               ""
#define MQTT_PASSWORD               ""

/* MQTT Topics */
#define MQTT_CMD_TOPIC              "pancake/robot/cmd"
#define MQTT_STATE_TOPIC            "pancake/robot/state"
#define MQTT_ERROR_TOPIC            "pancake/robot/error"
#define MQTT_HEARTBEAT_TOPIC        "pancake/robot/heartbeat"

/* Transport Band Topics (External System) */
#define MQTT_TRANSPORT_STATE_TOPIC  "pancake/transportband/state"

/* QoS Levels */
#define MQTT_CMD_QOS                (1)      // At-least-once
#define MQTT_STATE_QOS              (0)      // Fire-and-forget
#define MQTT_ERROR_QOS              (1)      // At-least-once
#define MQTT_HEARTBEAT_QOS          (0)      // Fire-and-forget

/* Client Configuration */
#define MQTT_CLIENT_IDENTIFIER      "psoc6-pancake-robot"
#define GENERATE_UNIQUE_CLIENT_ID   (1)
#define MQTT_TIMEOUT_MS             (5000)
#define MQTT_KEEP_ALIVE_SECONDS     (60)
```

### 2.2 wifi_config.h - WiFi Settings

```c
#define WIFI_SSID                   "NetworkName"
#define WIFI_PASSWORD               "Password"
#define WIFI_SECURITY               CY_WCM_SECURITY_WPA2

/* Connection Parameters */
#define WCM_CONNECT_TIMEOUT_MS      (10000)
#define WCM_CONNECT_RETRY_TIMES     (3)
#define WCM_CONNECT_RETRY_DELAY_MS  (5000)
```

---

## 3. Integratie Voorbeelden

### 3.1 Een Nieuw Commando Toevoegen

**Stap 1**: Definieer command type in `subscriber_task.h`:
```c
typedef enum {
    ROBOT_CMD_UNKNOWN = 0,
    ROBOT_CMD_MOVE = 1,
    ROBOT_CMD_STOP = 2,
    ROBOT_CMD_CLEAR_ERROR = 3,
    ROBOT_CMD_TEACH = 4,
    ROBOT_CMD_CUSTOM = 5        // ← NEW
} robot_cmd_t;
```

**Stap 2**: Parse command in `subscriber_task.c`:
```c
static robot_cmd_t parse_robot_cmd(const char *payload, int payload_len)
{
    // ... existing code ...
    
    if (strncmp(cmd_ptr, "custom", 6) == 0)
        return ROBOT_CMD_CUSTOM;    // ← NEW
    
    return ROBOT_CMD_UNKNOWN;
}
```

**Stap 3**: Handle command in `handle_robot_cmd()`:
```c
case ROBOT_CMD_CUSTOM:
{
    printf("Custom command received\n");
    // Implementeer je logica hier
    publish_state("custom_executed");
    break;
}
```

**Stap 4**: Publish naar broker:
```bash
mosquitto_pub -h 192.168.1.240 \
  -t "pancake/robot/cmd" \
  -m '{"cmd":"custom"}'
```

### 3.2 Een Externe Sensor Aansluiten

**Scenario**: Voeg een temperature sensor toe die state publiceert

**Stap 1**: Maak sensor header:
```c
// source/temperature_sensor.h
#ifndef TEMPERATURE_SENSOR_H_
#define TEMPERATURE_SENSOR_H_

void temp_sensor_init(void);
float temp_sensor_read(void);  // Returns °C

#endif
```

**Stap 2**: Integreer in subscriber task:
```c
// In subscriber_task.c
#include "temperature_sensor.h"

void subscriber_task(void *pvParameters)
{
    // ... bestaande code ...
    
    temp_sensor_init();  // Initialize sensor
    
    while (true)
    {
        // ... command handling ...
        
        // Lees temperature en publish periodiek
        static uint32_t last_temp_read = 0;
        uint32_t now = xTaskGetTickCount();
        
        if ((now - last_temp_read) > pdMS_TO_TICKS(10000))  // Elke 10s
        {
            float temp = temp_sensor_read();
            char temp_str[16];
            snprintf(temp_str, sizeof(temp_str), "%.1f", temp);
            
            // Publish via publisher task
            publisher_data_t cmd = {
                .cmd = PUBLISH_STATE,
                .state = temp_str
            };
            xQueueSend(publisher_task_q, &cmd, 0);
            
            last_temp_read = now;
        }
    }
}
```

### 3.3 Remote Monitoring Dashboard (Python/MQTT)

```python
#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import json
from datetime import datetime

# MQTT Broker Configuration
BROKER = "192.168.1.240"
PORT = 1883

# Message Handlers
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Subscribe to all robot topics
    client.subscribe("pancake/robot/#")

def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()
    timestamp = datetime.now().isoformat()
    
    print(f"[{timestamp}] {topic}: {payload}")
    
    # Parse JSON
    try:
        data = json.loads(payload)
        
        if "state" in data:
            print(f"  → Robot State: {data['state']}")
        
        if "error" in data:
            print(f"  → Error Status: {data['error']}")
            if "message" in data:
                print(f"     Message: {data['message']}")
        
        if "ts" in data:
            print(f"  → Timestamp: {data['ts']}")
    
    except json.JSONDecodeError:
        print("  ⚠ Invalid JSON")

# Main
if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    print(f"Connecting to MQTT broker at {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, 60)
    
    print("Listening for messages (press Ctrl+C to exit)...\n")
    client.loop_forever()
```

**Uitvoer**:
```
Connecting to MQTT broker at 192.168.1.240:1883...
Connected with result code 0
Listening for messages (press Ctrl+C to exit)...

[2026-05-01T10:23:45.123456] pancake/robot/state: {"state":"idle"}
  → Robot State: idle

[2026-05-01T10:23:50.456789] pancake/robot/heartbeat: {"ts":1609459200}
  → Timestamp: 1609459200

[2026-05-01T10:23:55.789012] pancake/robot/cmd: {"cmd":"move","pose":1}
  → (Received command from another client)
```

### 3.4 Batch Commands (Python/MQTT)

```python
#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import json
import time

def publish_command(client, cmd_type, pose=None):
    """Publish a robot command"""
    payload = {"cmd": cmd_type}
    
    if pose is not None:
        payload["pose"] = pose
    
    topic = "pancake/robot/cmd"
    client.publish(topic, json.dumps(payload), qos=1)
    print(f"Published: {topic} = {payload}")

def run_batch_sequence(broker, port):
    """Run a predefined sequence of commands"""
    
    client = mqtt.Client()
    client.connect(broker, port, 60)
    client.loop_start()
    
    try:
        # Sequence: Camera → Weigh → Mixer → Discard
        poses = [
            0,  # DEVICE_POSE_CAMERA
            2,  # DEVICE_POSE_SCALE
            3,  # DEVICE_POSE_MIXER
            4   # DEVICE_POSE_DISCARD
        ]
        
        for i, pose in enumerate(poses):
            print(f"\n[Step {i+1}] Moving to pose {pose}")
            publish_command(client, "move", pose)
            
            # Wait for movement to complete (check state topic)
            time.sleep(3)
        
        print("\n[Sequence Complete] Returning to camera pose")
        publish_command(client, "move", 0)
        
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    run_batch_sequence("192.168.1.240", 1883)
```

---

## 4. Error Codes & Return Values

### 4.1 cy_rslt_t Error Codes

```c
CY_RSLT_SUCCESS              (0x00000000)  // Operatie geslaagd
CY_RSLT_MODULE_GENERIC_ERROR (0x0FFFFFFF)  // Generieke module error

/* WiFi-specifieke errors */
CY_RSLT_WCM_INIT_FAILED      (0x...)  // WCM init error
CY_RSLT_WCM_CONNECT_FAILED   (0x...)  // WiFi connect error

/* MQTT-specifieke errors */
CY_RSLT_MQTT_INIT_ERROR      (0x...)  // MQTT init error
CY_RSLT_MQTT_CONNECT_ERROR   (0x...)  // MQTT connect error
CY_RSLT_MQTT_SUBSCRIBE_ERROR (0x...)  // Subscribe error
CY_RSLT_MQTT_PUBLISH_ERROR   (0x...)  // Publish error
```

### 4.2 FreeRTOS Return Values

```c
pdTRUE    (1)    // Operation succeeded, event occurred
pdFALSE   (0)    // Operation failed, timeout occurred
pdPASS    (1)    // Task creation succeeded
pdFAIL    (0)    // Task creation failed
```

---

## 5. Memory Pool Considerations

### 5.1 Task Stack Requirements

```c
// Profiling task stack usage:
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
printf("Task stack remaining: %d bytes\n", uxHighWaterMark);

// Rekommendaties:
#define MQTT_CLIENT_TASK_STACK_SIZE     (2048)  // Groot: WiFi callbacks
#define PUBLISHER_TASK_STACK_SIZE       (512)   // Klein: JSON formatting
#define SUBSCRIBER_TASK_STACK_SIZE      (1024)  // Medium: command parsing
#define CUSTOM_TASK_STACK_SIZE          (256)   // Minimal voor eenvoudige werk
```

### 5.2 Queue Memory Allocation

```c
// Queue creation
mqtt_task_q = xQueueCreate(MQTT_TASK_QUEUE_LENGTH,    // Depth
                           sizeof(mqtt_task_cmd_t));  // Item size

// Memory calculation:
// Total = Depth × ItemSize × overhead(~8 bytes per item)
// mqtt_task_q: 3 × 4 × 8 ≈ 96 bytes
```

---

## 6. Timing & Performance

### 6.1 Critical Timing Constraints

```c
// Motor Step Timing
#define MOTOR_STEP_PULSE_WIDTH_US   (5)     // MIN voor DRV8825
#define MOTOR_STEP_DELAY_US         (5)     // Hold time
#define MAX_MOTOR_SPEED_HZ          (100000) // Max 100 kHz

// MQTT Protocol Timing (Broker-level)
#define MQTT_KEEP_ALIVE_SEC         (60)    // MQTT PING every 60s (RFC 3.1.2.10)
#define MQTT_OPERATION_TIMEOUT_MS   (5000)  // Max wait for MQTT publish/subscribe response

// Application Heartbeat Timing (Custom)
// ⚠️ OPMERKING: HEARTBEAT_INTERVAL_MS (5000ms) ≠ MQTT_OPERATION_TIMEOUT_MS (5000ms)
//   - MQTT_OPERATION_TIMEOUT: Timeout per individuele MQTT operatie
//   - HEARTBEAT_INTERVAL: Hoe often applicatie heartbeat-berichten publiceert
#define HEARTBEAT_INTERVAL_MS       (5000)  // Publish heartbeat every 5s (app-level)
#define HEARTBEAT_TIMEOUT_FOR_ERROR (10000) // 10s no heartbeat = connection lost (monitoring)
```

**Timing Hierarchie Diagram**:
```
┌─────────────────────────────────────────────┐
│ MQTT KEEP_ALIVE (60 seconden)              │
│ └─ Automatische PING naar broker           │
│                                             │
│ ┌──────────────────────────────────────┐   │
│ │ Heartbeat Monitoring (10 seconden)   │   │
│ │ └─ Controleer of we data ontvangen   │   │
│ │                                       │   │
│ │ ┌──────────────────────────────────┐ │   │
│ │ │ Heartbeat Publish (5 seconden)   │ │   │
│ │ │ └─ Stuur heartbeat-bericht       │ │   │
│ │ │                                   │ │   │
│ │ │ ┌────────────────────────────┐   │ │   │
│ │ │ │ MQTT Op Timeout (5 sec)    │   │ │   │
│ │ │ │ └─ Timeout per publish/sub  │   │ │   │
│ │ │ └────────────────────────────┘   │ │   │
│ │ └──────────────────────────────────┘ │   │
│ └──────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

**Praktische Scenario**:
```
Tijdlijn (seconden):
────────────────────────────────────────────

0s    MQTT_OPERATION_TIMEOUT ---|
5s    HEARTBEAT PUBLISHED ✓
10s   HEARTBEAT PUBLISHED ✓
15s   HEARTBEAT PUBLISHED ✓
20s   MQTT_OPERATION_TIMEOUT ---|
...
60s   MQTT_KEEP_ALIVE PING naar broker
65s   HEARTBEAT PUBLISHED ✓
70s   HEARTBEAT PUBLISHED ✓
      (geen heartbeat ontvangen van broker)
80s   ⚠️ HEARTBEAT_TIMEOUT_FOR_ERROR: Connection lost!
```

### 6.2 Task Scheduling Analysis

```
Time (ms)   Event
─────────────────────────────────────
0           mqtt_client_task starts
10          WiFi scan begins
2010        WiFi scan complete
2020        MQTT client task created
2100        publisher_task created
2200        subscriber_task created
2210        Subscribe to topics
2230        Initial state published
5230        Heartbeat #1 published
10230       Heartbeat #2 published
...

Movement sequence (move to pose):
─────────────────────────────────────
0ms         Command received
10ms        parse_robot_cmd() returns
15ms        DeviceFunction_move() starts
215ms       Motor movement complete (1000 steps @ 10µs)
225ms       State "idle" published
```

---

## 7. Extensibility Points

### 7.1 Custom Input Devices

Voeg je eigen input handler toe:

```c
// custom_input.h
void custom_input_init(void);
void custom_input_task(void *pvParameters);
void custom_input_callback(void);  // Called on event
```

Integreer in `mqtt_client_task()`:
```c
xTaskCreate(custom_input_task, "custom_input", 512, NULL, 1, NULL);
```

### 7.2 Custom Output Actuators

Voeg je eigen output handler toe:

```c
// custom_output.h
typedef enum {
    CUSTOM_OUTPUT_RELAY = 0,
    CUSTOM_OUTPUT_VALVE = 1,
} custom_output_type_t;

void custom_output_init(custom_output_type_t type);
void custom_output_activate(custom_output_type_t type);
void custom_output_deactivate(custom_output_type_t type);
```

Oproep vanuit `subscriber_task`:
```c
case ROBOT_CMD_CUSTOM:
    custom_output_activate(CUSTOM_OUTPUT_RELAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
    custom_output_deactivate(CUSTOM_OUTPUT_RELAY);
    break;
```

### 7.3 Custom MQTT Topics

Voeg nieuwe topics toe in `mqtt_client_config.h`:
```c
#define MQTT_CUSTOM_TOPIC    "pancake/robot/custom"
#define MQTT_CUSTOM_QOS      (0)
```

Abonneer in `subscriber_task.c`:
```c
static cy_mqtt_subscribe_info_t subscribe_info[SUBSCRIPTION_COUNT] =
{
    // Bestaande topics...
    {
        .qos = (cy_mqtt_qos_t) MQTT_CUSTOM_QOS,
        .topic = MQTT_CUSTOM_TOPIC,
        .topic_len = (sizeof(MQTT_CUSTOM_TOPIC) - 1)
    }
};
```

---

## 8. Testing Frameworks

### 8.1 Unit Testing (C)

```c
// test/test_motor.c
#include <stdio.h>
#include <assert.h>
#include "../source/device_function.h"

void test_motor_initialization(void)
{
    DeviceFunction motor;
    DeviceFunction_init(&motor, "test_motor", P9_0, 0);
    
    assert(motor.name != NULL);
    assert(motor.is_moving == false);
    
    printf("✓ test_motor_initialization passed\n");
}

void test_pose_validation(void)
{
    assert(DEVICE_POSE_CAMERA == 0);
    assert(DEVICE_POSE_DISCARD == 4);
    assert(DEVICE_POSE_COUNT == 5);
    
    printf("✓ test_pose_validation passed\n");
}

int main(void)
{
    test_motor_initialization();
    test_pose_validation();
    
    printf("\n✓ All tests passed!\n");
    return 0;
}
```

### 8.2 Integration Testing (Python)

```python
# test/test_mqtt_integration.py
import unittest
import paho.mqtt.client as mqtt
import json
import time

class TestMQTTIntegration(unittest.TestCase):
    
    def setUp(self):
        self.broker = "192.168.1.240"
        self.client = mqtt.Client()
        self.received_messages = []
        
        self.client.on_message = self.on_message
        self.client.connect(self.broker, 1883, 60)
        self.client.loop_start()
    
    def on_message(self, client, userdata, msg):
        self.received_messages.append({
            'topic': msg.topic,
            'payload': json.loads(msg.payload.decode())
        })
    
    def test_heartbeat_received(self):
        """Test that heartbeat is received every 5 seconds"""
        self.client.subscribe("pancake/robot/heartbeat")
        time.sleep(6)  # Wait for heartbeat
        
        self.assertGreater(len(self.received_messages), 0)
        msg = self.received_messages[0]
        self.assertIn("ts", msg['payload'])
    
    def test_command_response(self):
        """Test that move command gets response"""
        self.client.subscribe("pancake/robot/state")
        
        # Send move command
        cmd = {"cmd": "move", "pose": 1}
        self.client.publish("pancake/robot/cmd", json.dumps(cmd), qos=1)
        
        time.sleep(1)
        
        # Check response
        states = [m for m in self.received_messages 
                  if m['topic'] == "pancake/robot/state"]
        self.assertGreater(len(states), 0)
    
    def tearDown(self):
        self.client.loop_stop()
        self.client.disconnect()

if __name__ == '__main__':
    unittest.main()
```

---

## 9. Checklisten voor Deployment

### 9.1 Pre-Release Checklist

- [ ] Alle header files opgeruimd en documented
- [ ] WiFi credentials ingesteld (`wifi_config.h`)
- [ ] MQTT broker adres correct (`mqtt_client_config.h`)
- [ ] Alle motor pins geverificeerd (`device_function.h`)
- [ ] Seriale baud rate: 115200 bps
- [ ] Debug output redirection working
- [ ] Alle tasks succesvol gecreëerd
- [ ] Geen compiler warnings
- [ ] MQTT heartbeat verschijnt elke 5 seconden
- [ ] Motor movement tests geslaagd

### 9.2 Runtime Validation

```bash
# 1. Check seriële output
screen /dev/ttyUSB0 115200
# Verwacht: "CE229889 - MQTT Client running on CM4"

# 2. Test WiFi verbinding
# Verwacht: "Connected to WiFi network successfully!"

# 3. Test MQTT verbinding
mosquitto_sub -h 192.168.1.240 -t "pancake/robot/heartbeat"
# Verwacht: {"ts":...} elke 5 seconden

# 4. Test command
mosquitto_pub -h 192.168.1.240 -t "pancake/robot/cmd" -m '{"cmd":"move","pose":0}'
mosquitto_sub -h 192.168.1.240 -t "pancake/robot/state"
# Verwacht: {"state":"idle"}
```

---

## Conclusie

Dit document biedt alle informatie nodig voor:
- Integratie van externe systemen
- Toevoeging van sensoren/actuators
- Uitbreiding met custom commando's
- Deployment en testing

Voor meer details, zie [README.md](README.md) en [TECHNICAL_GUIDE.md](TECHNICAL_GUIDE.md).
