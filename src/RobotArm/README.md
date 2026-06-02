# RobotArm - Pancake Robot MQTT Controller

RobotArm is een ModusToolbox-project voor een robotarm met drie stappenmotoren en een servo als end effector. De applicatie draait op FreeRTOS, maakt verbinding met WiFi en MQTT, verwerkt commando's via topics en publiceert status-, fout- en heartbeat-berichten.

## Projectoverzicht

- `source/main.c` start het systeem en lanceert `mqtt_client_task`.
- `source/mqtt_task.c` regelt WiFi, MQTT en reconnectie.
- `source/subscriber_task.c` ontvangt robotcommando's en transportbandstatus.
- `source/publisher_task.c` publiceert state, error en heartbeat berichten.
- `source/device_function.c` bestuurt de motoren, servo, CapSense en teach mode.

## Wat doet het systeem

Het systeem is bedoeld voor een robotarm in een industriële setup. In de code zie je deze functies terug:

- Een WiFi-verbinding opzetten met de instellingen uit `configs/wifi_config.h`.
- Verbinden met een MQTT-broker uit `configs/mqtt_client_config.h`.
- Commando's ontvangen op `pancake/robot/cmd`.
- Status publiceren op `pancake/robot/state`.
- Fouten publiceren op `pancake/robot/error`.
- Heartbeats publiceren op `pancake/robot/heartbeat`.
- Transportbandstatus lezen op `pancake/transportband/state`.
- Poses opslaan via teach mode met de save button en CapSense.

## Hardware

De code gebruikt deze pinnen:

| Functie | Pin | Code-naam |
|---|---|---|
| Motor 1 STEP | P9_0 | `STEP1_PIN` |
| Motor 1 DIR | P9_1 | `DIR1_PIN` |
| Motor 1 EN | P9_2 | `EN1_PIN` |
| Motor 2 STEP | P10_0 | `STEP2_PIN` |
| Motor 2 DIR | P10_1 | `DIR2_PIN` |
| Motor 2 EN | P10_2 | `EN2_PIN` |
| Motor 3 STEP | P10_3 | `STEP3_PIN` |
| Motor 3 DIR | P10_4 | `DIR3_PIN` |
| Motor 3 EN | P10_5 | `EN3_PIN` |
| Servo PWM | P10_6 | `SERVO_PIN` |
| Save button | P0_4 | `SAVE_BUTTON_PIN` |

Debug output loopt via de retargeted UART van de BSP.

## Setup

1. Open het project in ModusToolbox.
2. Stel je WiFi-gegevens in in `configs/wifi_config.h`.
3. Controleer het MQTT-brokeradres in `configs/mqtt_client_config.h`.
4. Pas indien nodig de topic- en QoS-instellingen aan in dezelfde configuratie.
5. Bouw het project en flash het naar het bord.
6. Open de seriële monitor op 115200 baud.

Belangrijke standaardwaarden uit de code:

| Instelling | Waarde |
|---|---|
| MQTT broker | `192.168.1.240` |
| MQTT poort | `1883` |
| MQTT keep-alive | `60` seconden |
| Secure MQTT | uit (`0`) |
| Client ID | `psoc6-pancake-robot` |

## Gebruik

### Opstarten

Na het starten zie je in de UART-output dat het board initialiseert, WiFi verbindt en MQTT opzet. Daarna wordt de subscriber task gestart en publiceert het systeem de eerste idle state.

### Gebruikersworkflow

Voor normaal gebruik kun je deze volgorde aanhouden:

1. Start het bord en wacht tot de UART meldt dat WiFi en MQTT verbonden zijn.
2. Open een MQTT-client zoals `mosquitto_pub` en `mosquitto_sub`.
3. Publiceer een commando naar `pancake/robot/cmd`.
4. Lees de reactie op `pancake/robot/state` of `pancake/robot/error`.
5. Controleer of er nog heartbeats binnenkomen op `pancake/robot/heartbeat`.

Voorbeeld van een normale werkstroom:

```bash
mosquitto_pub -h 192.168.1.240 -t "pancake/robot/cmd" -m '{"cmd":"teach","pose":"camera"}'
mosquitto_pub -h 192.168.1.240 -t "pancake/robot/cmd" -m '{"cmd":"save"}'
mosquitto_pub -h 192.168.1.240 -t "pancake/robot/cmd" -m '{"cmd":"goto","pose":"camera"}'
```

Als je alleen wilt meekijken, kun je alle robotberichten volgen met:

```bash
mosquitto_sub -h 192.168.1.240 -t "pancake/robot/#" -v
```

### MQTT-commando's

De subscriber verwerkt deze commando's:

| Command | Doel |
|---|---|
| `teach` | Activeer teach mode voor een pose |
| `goto` | Beweeg naar een opgeslagen pose |
| `save` | Sla de huidige motorpositie op |
| `stop` | Stop de actie en ga terug naar idle |

Voorbeelden van payloads die passen bij de huidige parser:

```json
{ "cmd": "teach", "pose": "camera" }
{ "cmd": "goto", "pose": "scale" }
{ "cmd": "save" }
{ "cmd": "stop" }
```

### Topics

| Topic | Gebruik |
|---|---|
| `pancake/robot/cmd` | Commando's naar de robotarm |
| `pancake/transportband/state` | Status van de transportband |
| `pancake/robot/state` | Status van de robotarm |
| `pancake/robot/error` | Foutmeldingen |
| `pancake/robot/heartbeat` | Periodieke heartbeat |

### Teach mode

Teach mode gebruikt de save button en CapSense:

- Button 0 beweegt de actieve motor in de positieve richting.
- Button 1 beweegt de actieve motor in de negatieve richting.
- P0_4 slaat de huidige positie op en gaat naar de volgende motor.

De beschikbare poses zijn:

| Pose | Index |
|---|---:|
| camera | 0 |
| inductive | 1 |
| scale | 2 |
| mixer | 3 |
| discard | 4 |

### Monitoring

De publisher task stuurt elke 5000 ms een heartbeat naar `pancake/robot/heartbeat`. Daarmee kun je snel zien of het systeem nog leeft en of de broker bereikbaar is.

## Wat is echt geïmplementeerd

- WiFi-connectie en reconnectie in `mqtt_task.c`.
- MQTT-connect, disconnect-cleanup en her-abonneren in `mqtt_task.c`.
- State-, error- en heartbeat publishing in `publisher_task.c`.
- Command parsing voor `teach`, `goto`, `save` en `stop` in `subscriber_task.c`.
- Motorbesturing voor drie stappenmotoren in `device_function.c`.
- Servo-aansturing op `P10_6`.
- CapSense-ondersteuning voor teach mode.
- Koppeling met transportbandstatus via `pancake/transportband/state`.

## Troubleshooting

| Probleem | Mogelijke oorzaak | Controle |
|---|---|---|
| WiFi maakt geen verbinding | Verkeerde SSID of wachtwoord | `configs/wifi_config.h` |
| MQTT verbindt niet | Brokeradres of poort fout | `configs/mqtt_client_config.h` |
| Geen state of heartbeat | Publisher task draait niet of broker is onbereikbaar | UART-output en topicnaam |
| Teach mode reageert niet | CapSense of board-config is niet juist | `source/device_function.c` en BSP |
| Save button doet niets | Verkeerde pin of bedrading | `SAVE_BUTTON_PIN` |

## Meer details

- [TECHNICAL_GUIDE.md](TECHNICAL_GUIDE.md) voor interne werking en achtergrond.
- [API_GUIDE.md](API_GUIDE.md) als je zelf nieuwe functies, sensors of commando's wilt toevoegen.
