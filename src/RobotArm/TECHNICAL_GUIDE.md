# Technische Handleiding - RobotArm MQTT Controller

## Inleiding

Deze technische handleiding biedt gedetailleerde informatie over de interne werking van het RobotArm MQTT-besturingssysteem. Dit document beschrijft de hardware-configuratie, FreeRTOS-taakstructuur, WiFi/MQTT-communicatie en de implementatie van motorbesturing met teachmode.

---

## 1. Hardware-Configuratie & Pinouts

### 1.1 Stappenmotoren Pinouts

#### Motor 1:
| Functie | Pin | Type | Beschrijving |
|---------|-----|------|------------|
| STEP | P9_0 | GPIO Output | Stappuls |
| DIR | P9_1 | GPIO Output | Richting |
| EN | P9_2 | GPIO Output | Enable (0=actief, 1=uit) |

#### Motor 2:
| Functie | Pin | Type | Beschrijving |
|---------|-----|------|------------|
| STEP | P10_0 | GPIO Output | Stappuls |
| DIR | P10_1 | GPIO Output | Richting |
| EN | P10_2 | GPIO Output | Enable |

#### Motor 3:
| Functie | Pin | Type | Beschrijving |
|---------|-----|------|------------|
| STEP | P10_3 | GPIO Output | Stappuls |
| DIR | P10_4 | GPIO Output | Richting |
| EN | P10_5 | GPIO Output | Enable |

### 1.2 Servo & Accessoires

| Functie | Pin | Type | Frequentie | Beschrijving |
|---------|-----|------|----------|-----------|
| Servo PWM | P10_6 | PWM Output | 50 Hz | Servo besturing |
| Save Button | P0_4 | GPIO Input | - | Teachmode trigger |
| Debug UART | UART0 | Serial | 115200 baud | Console output |

### 1.3 Poses (Teachmode)

Het systeem ondersteunt 5 opgeslagen poses per motor:

| Pose | Index | Beschrijving |
|------|-------|------------|
| CAMERA | 0 | Camera positie |
| INDUCTIVE | 1 | Inductieve sensor |
| SCALE | 2 | Weegschaal positie |
| MIXER | 3 | Mengmachine positie |
| DISCARD | 4 | Afval positie |

---

## 2. FreeRTOS Real-Time Operating System

### 2.1 Taakstructuur

Het systeem gebruikt preemptieve prioriteitsplanning met drie applicatietaken:

| Task | Prioriteit | Stack | Beschrijving |
|------|-----------|-------|------------|
| mqtt_client_task | 2 | 2048 bytes | WiFi/MQTT verbinding en beheer |
| publisher_task | 2 | 1024 bytes | Status/error publishing |
| subscriber_task | 1 | 1024 bytes | Command ontvangst en verwerking |

### 2.2 Communicatie tussen Tasks

Tasks communiceren via FreeRTOS queues:

```
publisher_task_q
├─ PUBLISHER_INIT
├─ PUBLISHER_DEINIT
├─ PUBLISH_STATE
└─ PUBLISH_ERROR

subscriber_task_q
├─ Commands van MQTT
└─ Motorcommando's
```

### 2.3 Interrupt Service Routines (ISRs)

| Bron | Prioriteit | Handler | Beschrijving |
|------|-----------|---------|------------|
| CapSense | 7 | capsense_isr | Touchpad scanning |
| WiFi MAC | 6 | cy_wcm_wifi_isr | WiFi RX/TX |
| UART Debug | 5 | cyhal_uart_isr | Console output |
| System Tick | 2 | SysTick_Handler | FreeRTOS context switch |

**ISR-richtlijnen:**
- ISRs mogen NIET blocking (vTaskDelay, xQueueReceive)
- Gebruik xQueueSendFromISR() voor queue operaties
- Maximale duur: < 1ms

---

## 3. WiFi & MQTT Protocol Stack

### 3.1 WiFi Verbindingsflow

```
┌──────────────────────────────────────────────────┐
│  WiFi Initialization & Connection Sequence       │
└──────────────────────────────────────────────────┘

[1] cy_wcm_init(&config)
    └─ Initialize WiFi subsystem
       ├─ Load WiFi firmware (from ROM or QSPI XIP)
       ├─ Initialize MAC/PHY layers
       └─ Register event callbacks

    ↓

[2] Start WiFi Scan (optional)
    └─ Scan for available APs
       └─ Duur: ~2 seconden

    ↓

[3] cy_wcm_connect_ap(&ap_credentials)
    ├─ Negotiate with AP:
    │  ├─ Probe Request/Response
    │  ├─ Authentication (4-way handshake)
    │  └─ Association
    │
    └─ Timeout: 10 seconden (default)
       └─ Retry tot 3x met 5s delay

    ↓

[4] DHCP Client (lwIP integration)
    └─ Request IP from AP
       ├─ Duur: ~2 seconden
       ├─ Lease: 24 uren (typisch)
       └─ Renewal: automatisch

    ↓

[5] WiFi Connected Event
    └─ Signal for MQTT connection
```

**WiFi Parameters** (uit `wifi_config.h`):
```c
#define WIFI_SSID           "NetworkName"
#define WIFI_PASSWORD       "Password123"
#define WIFI_SECURITY       CY_WCM_SECURITY_WPA2  // WPA2/WPA3
#define WIFI_AP_CH_HINT     0                     // Auto-detect
#define WIFI_STATIC_IP      0                     // Use DHCP
```

### 3.2 MQTT Protocol & Topics

**MQTT Version**: 3.1.1 (nicht 5.0)

**Topic Hierarchie**:
```
pancake/                          [Root namespace]
├── robot/                        [Robot subsystem]
│   ├── cmd                       [TOPIC: Commands TO robot]
│   │   Type: Subscribe (Rx)
│   │   QoS: 1 (At-least-once)
│   │   Example: {"cmd":"move","pose":1}
│   │
│   ├── state                     [TOPIC: Robot state FROM robot]
│   │   Type: Publish (Tx)
│   │   QoS: 0 (Fire-and-forget)
│   │   Example: {"state":"idle"}
│   │
│   ├── error                     [TOPIC: Error reports FROM robot]
│   │   Type: Publish (Tx)
│   │   QoS: 1 (At-least-once)
│   │   Example: {"error":true,"message":"Motor stall"}
│   │
│   └── heartbeat                 [TOPIC: Keep-alive FROM robot]
│       Type: Publish (Tx)
│       QoS: 0 (Fire-and-forget)
│       Interval: 5 seconden
│       Example: {"ts":1609459200}
│
└── transportband/                [External system: Transport band]
    └── state                     [TOPIC: Transport status]
        Type: Subscribe (Rx)
        QoS: 0
        Example: {"status":"moving","pos":42}
```

**MQTT Handshake Sequence**:

```
Device                          MQTT Broker
  │                                 │
  ├──── CONNECT ─────────────────→ │
  │ (client_id, username, pwd,     │
  │  keep_alive=60s, will_msg,     │
  │  clean_session=1)              │
  │                                 │
  │                            [Auth check]
  │                                 │
  │ ←─── CONNACK ───────────────── │
  │ (return_code=0 [OK])           │
  │                                 │
  ├──── SUBSCRIBE ───────────────→ │
  │ (pancake/robot/cmd, qos=1)     │
  │ (pancake/transportband/state)  │
  │                                 │
  │ ←─── SUBACK ───────────────── │
  │                                 │
  ├──── PUBLISH ───────────────→  │
  │ (pancake/robot/state)          │
  │ (qos=0)                        │
  │                                 │
  │ PING/PONG ↔ (every 60s)       │
  │                                 │
  │ ← MESSAGE ─ (cmd received)     │
  │                                 │
  ├──── PUBACK ───────────────→   │
  │ (acknowledge message delivery) │
  │                                 │
  │ ← DISCONNECT ─ (if timeout)    │
```

### 3.3 QoS Levels

| QoS | Beschrijving | Gebruik |
|-----|-------------|--------|
| 0 | Fire-and-Forget | State updates |
| 1 | At-Least-Once | Commands, Errors |

---

## 4. Motorbesturing & Teachmode

### 4.1 Stappenmotor Aansturing

Elke motor heeft drie GPIO-pins:
- **STEP**: Genereert stappulsen (2ms hoog, 2ms laag)
- **DIR**: Bepaalt rotatie richting
- **EN**: Enable motor (0=actief, 1=uit)

**Positie Tracking:**
- Motor 1-3 geven huidige stapcount in `M1_Pos`, `M2_Pos`, `M3_Pos`
- Positie loopt van 0 tot maximale stappen

### 4.2 Servo Besturing

PWM-signaal op P10_6:
- Frequentie: 50 Hz (20ms periode)
- Pulsbreedten: 1-2ms bepalen hoek
- Startpositie wordt ingesteld via `Servo_startpositie()`

### 4.3 Teachmode

**Operatie:**
1. Druk op Save Button (P0_4) om teach mode in te schakelen
2. Gebruik CapSense pads om motoren te bedienen
3. Druk Save Button weer om pose op te slaan

**Opgeslagen Poses:**
- Camera, Inductive, Scale, Mixer, Discard
- 5 poses × 3 motoren = 15 positiewaarden totaal

---

## 5. MQTT Communicatie

### 5.1 Command Payloads

Topic: `pancake/robot/cmd`

```json
{
  "cmd": "move",
  "pose": 0
}
```

Geldige poses: 0=CAMERA, 1=INDUCTIVE, 2=SCALE, 3=MIXER, 4=DISCARD

### 5.2 State Payloads

Topic: `pancake/robot/state`

```json
{
  "state": "idle"
}
```

Geldige states: `idle`, `moving`, `pose_saved_motor_0/1/2`

### 5.3 Error Payloads

Topic: `pancake/robot/error`

```json
{
  "error": false,
  "message": "OK"
}
```

---

## 6. Debugging & Troubleshooting

### 6.1 Serial Debug Output

**UART Configuratie:**
- Baud Rate: 115200 bps
- Data Bits: 8, Stop Bits: 1
- Parity: None, Flow Control: None

**Debug Console Commando's:**
```bash
# Monitor motor positions (via printf output)
# [MOTOR] M1_Pos: 100, M2_Pos: 50, M3_Pos: 75

# WiFi debug messages
# [WiFi] Connected! IP: 192.168.x.x

# MQTT connection status
# [MQTT] Connected to broker
```

### 6.2 MQTT Debugging

```bash
# Monitor MQTT topics op broker
mosquitto_sub -h <broker-ip> -t "pancake/#" -v

# Filter specifieke topic
mosquitto_sub -h <broker-ip> -t "pancake/robot/state"
```

### 6.3 Problemen Oplossen

| Probleem | Oorzaak | Oplossing |
|----------|--------|----------|
| Motor beweegt niet | GPIO niet geïnitialiseerd | Check pin definities in device_function.h |
| WiFi timeout | Zwak signaal of antenne | Dichter bij router, antenne controleren |
| MQTT connect fails | Broker offline/firewall | Broker IP en firewall regels verifiëren |
| Servo jittert | Interrupt latency | Timer interrupts checken |

---

## 7. Referenties

- **Infineon PSOC6**: https://www.infineon.com/psoc6
- **ModusToolbox**: https://www.infineon.com/modustoolbox
- **FreeRTOS Kernel**: https://www.freertos.org/
- **MQTT Specification**: https://mqtt.org/mqtt-specification

---
