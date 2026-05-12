# TwinCAT 3 PLC - Technisch Documentatieverslag

## 1. Projectoverzicht

### Doel
Dit TwinCAT 3 PLC-project bestuurt een pannenkoekenproces met een transportband, robotarm, MQTT-communicatie en een QR-vision module. Het PLC-programma stuurt de transportband, publiceert transportbandstatus via MQTT en verstuurt robotarm-commando's (teach/goto/save/stop) naar een MQTT-broker. Daarnaast worden robotstatus, error en heartbeat via MQTT ingelezen voor visualisatie en statuslogica.

Het systeem is ontworpen als centrale orkestratie-laag: de PLC bepaalt de procesflow, externe devices voeren mechanische acties uit. MQTT fungeert als integratielaag voor subsystemen die buiten het PLC-project draaien (robotarm en mixer) en voor visualisatie.

### Hardware-architectuur (hoog niveau)
- PLC runtime: TwinCAT 3 (XAE / Runtime op Windows).
- MQTT-communicatie: PLC fungeert als MQTT client, topics voor robot, transportband en mixer.
- Transportband: logica intern gesimuleerd met timer in PLC (vervangbaar door echte I/O).
- Robotarm: extern device, aangestuurd via MQTT, status terug via MQTT.
- Vision: QR code detectie via TwinCAT Vision (GigE camera control).

Concrete I/O en EtherCAT mapping worden niet in de PLC code definities aangetroffen (geen I/O mapping in GVL_Inputs/Outputs). Zie sectie 2 voor details.

## 2. Systeemconfiguratie

### I/O mapping
In de huidige projectbestanden zijn geen fysieke I/O variabelen gedefinieerd:
- [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Inputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Inputs.TcGVL)
- [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Outputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Outputs.TcGVL)

De transportband wordt functioneel gesimuleerd in [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU) via een timer. Daardoor is de transportband-actie volledig deterministisch binnen de PLC-cyclus en eenvoudig te vervangen door echte I/O.

Logische I/O in dit project:
- VISU inputs (knoppen) sturen procescommando's naar de PLC.
- MQTT topics vormen de logische I/O naar externe subsystemen (robotarm, transportband, mixer).

### EtherCAT devices
In de PLC bronbestanden zijn geen specifieke EtherCAT I/O devices of mappings teruggevonden. Eventuele EtherCAT configuratie zit buiten de PLC code (in TwinCAT System Manager config), die niet in de repo zichtbaar is.

### Task-instellingen (cycle times)
In de PLC POUs zijn geen expliciete cycle times of task configuratie gedeclareerd. Deze staan meestal in de TwinCAT projectconfiguratie (System Manager). In de repo is alleen de PLC code opgenomen.

Conceptuele timing (impliciet uit de code):
- PLC-cyclus bepaalt de resolutie van commando-pulsen (rising-edge detectie met R_TRIG in MAIN).
- MQTT publish/subscribe wordt per PLC-cyclus afgehandeld in FB_MQTT, met interne message queue.
- Transportband-simulatie gebruikt een TON met 3s looptijd.
- Robot-heartbeat watchdog werkt met 12s timeout en wordt per cyclus bijgewerkt.

## 3. Software-architectuur

# TwinCAT 3 PLC - API & Technische Handleiding

## Inleiding

Dit document beschrijft de PLC-architectuur, publieke interfaces en integratiepunten van het TwinCAT 3 project. De stijl en opbouw volgen de RobotArm documentatie (README, API_GUIDE, TECHNICAL_GUIDE) zodat de projectdocumentatie consistent blijft.

---

## 1. Projectoverzicht

Dit TwinCAT 3 PLC-project bestuurt een pannenkoekenproces met een transportband, robotarm, MQTT-communicatie en een QR-vision module. De PLC fungeert als centrale orkestratie-laag: het procesverloop wordt in de PLC bepaald en externe subsystemen (robotarm, mixer) worden via MQTT aangestuurd.

Hoofdonderdelen:
- Transportband flow via PLC state machine en FB_Transportband.
- Robotarm commando's via MQTT (`teach`, `goto`, `save`, `stop`).
- QR-vision via TwinCAT Vision API.

Belangrijkste integratiepunten:
- VISU knoppen sturen commando-pulsen (R_TRIG) in MAIN.
- MQTT is de enige interface naar robotarm/mixer en transportband status.
- De PLC bewaart status en display-strings in GVL_MQTT voor de visualisatie.

---

## 2. Architectuur & Moduleoverzicht

### 2.1 POU Structuur

- Program:
  - MAIN
- Function Blocks:
  - FB_MQTT
  - FB_StateMachine
  - FB_Transportband
  - FB_QrVision
- Functions:
  - F_ParseStateDisplay
  - F_ParseErrorDisplay

Hoofdflow (vereenvoudigd):

```
MAIN
├─ FB_StateMachine   (processtatus)
├─ FB_Transportband  (transportcyclus)
├─ FB_MQTT           (MQTT publish/subscribe)
└─ FB_QrVision       (QR detectie)
```

### 2.2 Besturingslogica

- MAIN vertaalt VISU input naar commando-pulsen met `R_TRIG`.
- Transportband start/stop wordt gelatcht zodat korte taps een volledige cyclus starten.
- Robotarm commands krijgen prioriteit (stop > save > teach > goto).
- Statusvelden voor visualisatie worden uit MQTT-parse resultaten opgebouwd.

Timing en determinisme:
- Alle commando's worden in een PLC-cyclus afgehandeld.
- MQTT publish en subscribe worden sequentieel afgehandeld in FB_MQTT.
- Transportband loopt via een TON (3s) en staat los van de robotarm flow.

---

## 3. MQTT Integratie

### 3.1 Broker & Topics

Broker en topics staan in [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_MQTT.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_MQTT.TcGVL):

```
sMqttBroker := '192.168.1.240'
nMqttPort := 1883

sTopicRobotCmd := 'pancake/robot/cmd'
sTopicRobotState := 'pancake/robot/state'
sTopicRobotError := 'pancake/robot/error'
sTopicRobotHeartbeat := 'pancake/robot/heartbeat'

sTopicTransportbandState := 'pancake/transportband/state'
```

### 3.2 Command Payloads

```json
{ "cmd": "teach", "pose": "camera" }
{ "cmd": "goto",  "pose": "scale" }
{ "cmd": "save" }
{ "cmd": "stop" }
```

### 3.3 State & Error Payloads

Robot state:
```json
{ "state": "idle" }
{ "state": "running" }
{ "state": "done" }
{ "state": "at_camera" }
{ "state": "teach_camera" }
```

Robot error:
```json
{ "state": "ok",    "error": false, "message": "" }
{ "state": "error", "error": true,  "message": "unknown command" }
```

### 3.4 Publish & Subscribe Flow

- FB_MQTT configureert de MQTT client en subscribe't op robot, transportband en mixer topics.
- Robot commando's worden gepubliceerd bij VISU trigger (QoS 1).
- Transportband status wordt gepubliceerd bij verandering (QoS 0).
- Robot status/error wordt geparsed en vertaald naar PLC flags.

Flow (in woorden):
- VISU -> MAIN zet command flags -> FB_MQTT publiceert JSON.
- Robotarm publiceert state/error/heartbeat -> FB_MQTT parse -> GVL_MQTT status.
- Transportband status wordt continu gepubliceerd zolang de transportband actief of klaar is.

---

## 4. Logica per Function Block

### 4.1 MAIN

Belangrijke variabelen:
- `bStartCmd`, `bStopCmd` (transportband)
- `bRobotTeachCmd`, `bRobotGotoCmd`, `bRobotSaveCmd`, `bRobotStopCmd` (robotarm)
- `GVL_MQTT.sRobotArmDisplay`, `GVL_MQTT.sTransportbandDisplay` (visualisatie)

Hoofdroutine:
- Rising edges van VISU buttons worden omgezet naar eenmalige commando-pulsen.
- Robotarm status en transportband status worden elke cyclus naar display strings vertaald.

Command prioriteit (waarom):
- `stop` is altijd dominant om een lopend proces direct te kunnen stoppen.
- `save` wordt hoger geprioriteerd dan `teach`/`goto` om het teach-proces niet te onderbreken.
- `teach` start de teach flow, `goto` voert een pose verplaatsing uit.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/MAIN.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/MAIN.TcPOU)

### 4.2 FB_StateMachine

State model (E_MainState):
- INIT -> IDLE
- IDLE -> RUNNING (bStart of bTransportbandStart)
- RUNNING -> STOPPING (bStop)
- STOPPING -> IDLE
- ERROR -> IDLE (bStop)

Interpretatie:
- `RUNNING` is een generieke processtatus en staat los van robotarm of QR.
- `STOPPING` is een overgangsstaat die direct naar `IDLE` gaat.
- `ERROR` is sticky totdat een stop actie plaatsvindt.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_StateMachine.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_StateMachine.TcPOU)

### 4.3 FB_Transportband

Timer-gedreven simulatie:
- R_TRIG detecteert startpuls.
- `bCycleActive` houdt de cyclus vast.
- TON timer bepaalt `bDone` na 3s.
- `bStop` reset de cyclus.

Waarom dit zo is opgezet:
- Simulatie maakt het mogelijk om de rest van het systeem te testen zonder fysieke I/O.
- De latch voorkomt dat een korte startpuls slechts een fractie van de cyclus activeert.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU)

### 4.4 FB_MQTT

Hoofdrollen:
- MQTT client configuratie, connect, subscribe.
- Publish van transportband state/error payloads.
- Publish van robot commands bij VISU triggers.
- Parse van robot state/error/heartbeat.

Belangrijke status flags:
- `bRobotArmOnline` via heartbeat watchdog.
- `bRobotArmBusy`, `bRobotArmDone`, `bRobotArmTeachMode` via state parsing.
- `bRobotArmAlarm` via error parsing.

Publish details:
- Transportband state wordt alleen gepubliceerd als de payload is veranderd (change detection).
- Robot commando's zijn edge-triggered, zodat elk commando exact 1 keer per klik wordt verzonden.

Subscribe details:
- Messages worden uit een queue gehaald; parsing gebeurt sequentieel.
- Robot state strings zoals `teach_*`, `at_*`, `running`, `done`, `idle` bepalen de interne flags.
- Heartbeat wordt gebruikt voor een watchdog (12s) om offline status te detecteren.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_MQTT.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_MQTT.TcPOU)

### 4.5 FB_QrVision

QR detectie flow (geimplementeerde pipeline):
- Camera state wordt opgehaald via `fbCameraControl.GetState()` en alleen in `TCVN_CS_ACQUIRING` wordt beeldverwerking gedaan.
- Bij `bEnable = TRUE` wordt de acquisitie gestart en elk frame opgehaald met `GetCurrentImage()`.
- De grijswaarden input wordt gekopieerd naar een displayable image zodat de HMI een live beeld kan tonen.
- Voor QR detectie worden drie opeenvolgende pogingen gedaan met verschillende search strategies:
  - `TCVN_CSS_ONLY_NOT_INVERTED + TCVN_CSS_ONLY_NOT_FLIPPED`
  - `TCVN_CSS_ONLY_NOT_FLIPPED`
  - `TCVN_CSS_ONLY_NOT_INVERTED`
  Dit verhoogt de robuustheid tegen rotatie en contrastvariaties.
- Bij een succesvolle decode wordt `F_VN_ExportSubContainer_String()` gebruikt om de gedecodeerde QR string op te halen.
- Daarna wordt de input geconverteerd naar RGB (`F_VN_ConvertColorSpace`) en worden contouren getekend rond de QR (rood, dikte 3) via `F_VN_DrawContours`.
- Over het beeld wordt een overlay tekst geplaatst met `F_VN_PutTextExp`, zodat direct zichtbaar is wat er gedecodeerd is.
- Bij geen detectie wordt `sDisplayText` gezet op "No QR Code Found".
- `hrLast` bewaart de laatste HRESULT, zodat fouten in de vision pipeline traceerbaar blijven.

Belangrijkste outputs:
- `bCodeFound` geeft aan of er in dit frame een QR is gedetecteerd.
- `sCodeAsString` bevat de gedecodeerde inhoud.
- `sDisplayText` bevat de tekst die op het beeld wordt gerenderd.

Belangrijkste inputs:
- `bEnable` schakelt de volledige QR pipeline aan/uit.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_QrVision.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_QrVision.TcPOU)

---

## 5. Data Types & GVLs

### 5.1 GVL_MQTT

De GVL_MQTT is zowel configuratie als runtime status:
- Broker/port/client ID
- Topics (robot/transportband/mixer)
- VISU inputs (teach/goto/save/stop)
- Status flags en display strings

Waarom hier zoveel in staat:
- MQTT is de centrale integratiebus, dus configuratie en status moeten toegankelijk zijn.
- Visualisatie leest rechtstreeks status en display strings uit GVL_MQTT.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_MQTT.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_MQTT.TcGVL)

### 5.2 GVL_Data

Bevat QR-vision status en output:
- bQrEnabled, bQrCodeFound, sQrCodeText, sQrDisplayText, hrQrVision

Gebruik:
- De VISU kan `sQrDisplayText` tonen.
- `hrQrVision` helpt bij diagnose wanneer de camera of decode faalt.

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Data.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Data.TcGVL)

### 5.3 GVL_Inputs / GVL_Outputs

Leeg in de huidige code. Fysieke I/O mapping gebeurt extern in TwinCAT System Manager.

Bronnen:
-- [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Inputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Inputs.TcGVL)
-- [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Outputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Outputs.TcGVL)

### 5.4 DUTs

`E_MainState` definieert de processtates:
- INIT, IDLE, RUNNING, STOPPING, ERROR

Bron: [plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/DUTs/E_MainState.TcDUT](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/DUTs/E_MainState.TcDUT)

---

## 6. Veiligheid & Foutafhandeling

- Stop/noodstop wordt verwerkt via bStop in FB_StateMachine en FB_Transportband.
- Transportband reset volledig bij stop.
- Robot errors worden via MQTT parsed en als alarmstatus in de GVL gezet.
- Heartbeat watchdog markeert robot offline na 12s.

Belangrijk: fysieke safety (E-stop, safe I/O, safe torque off) zit niet in deze PLC code en moet apart worden geborgd.

Foutpaden:
- Als de MQTT broker wegvalt, reset FB_MQTT zijn subscribe flags en worden statusvelden gewist.
- Als robot heartbeat niet binnenkomt, wordt `bRobotArmOnline` FALSE.
- Robot error payloads zetten `bRobotArmAlarm` TRUE en overschrijven display strings.

---

## 7. Voorbeeld Integratie Flow

### 7.1 VISU -> MQTT -> Robot

1. Operator klikt "Teach" in VISU.
2. MAIN zet `bRobotTeachCmd`.
3. FB_MQTT publiceert `{"cmd":"teach","pose":"camera"}` op `pancake/robot/cmd`.
4. Robotarm publiceert `{"state":"teach_camera"}` op `pancake/robot/state`.
5. FB_MQTT zet `bRobotArmTeachMode = TRUE` en visualisatie toont TEACH.

Varianten:
- `goto` zorgt voor een `at_<pose>` state en zet `bRobotArmDone` TRUE.
- `save` houdt de robot in teach mode en bevestigt motor-stappen.

### 7.2 Transportband cyclus

1. Operator klikt "Start".
2. FB_Transportband start timer (3s).
3. MQTT publish `{"state":"running"}` naar `pancake/transportband/state`.
4. Na 3s publish `{"state":"done"}`.

Als de operator stopt:
- `bStop` reset direct de timer en publiceert weer `{"state":"idle"}`.

---

## 8. Debugging & Troubleshooting

### MQTT monitoring

```bash
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h 192.168.1.240 -t "pancake/#" -v
```

### Verwachte topics

- `pancake/robot/cmd`
- `pancake/robot/state`
- `pancake/robot/error`
- `pancake/robot/heartbeat`
- `pancake/transportband/state`

---

## 9. Referenties

- TwinCAT 3 XAE
- TwinCAT MQTT TF
- TwinCAT Vision (TcVision)
