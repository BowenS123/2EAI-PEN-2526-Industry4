# TwinCAT 3 PLC - Technisch documentatieverslag

## 1. Projectoverzicht

### Doel
Dit TwinCAT 3 PLC-project bestuurt een pannenkoekenproces met een transportband, robotarm, MQTT-communicatie en een QR-visionmodule. Het PLC-programma stuurt de transportband aan, publiceert de transportbandstatus via MQTT en verstuurt robotarmcommando's zoals teach, goto, save en stop naar een MQTT-broker. Daarnaast leest het robotstatus, foutstatus en heartbeat via MQTT in voor visualisatie en statuslogica.

Het systeem is opgezet als centrale orkestratielaag: de PLC bepaalt de procesflow en externe apparaten voeren de mechanische acties uit. MQTT fungeert als integratielaag voor subsystemen buiten het PLC-project, zoals de robotarm en de mixer, en voor visualisatie.

### Hardware-architectuur
- PLC-runtime: TwinCAT 3 op Windows.
- MQTT-communicatie: de PLC fungeert als MQTT-client met topics voor robot, transportband en mixer.
- Transportband: logica wordt intern in de PLC gesimuleerd met een timer en kan later worden vervangen door echte I/O.
- Robotarm: extern apparaat dat via MQTT wordt aangestuurd en status terugstuurt via MQTT.
- Vision: QR-code-detectie via TwinCAT Vision met GigE-camera.

Concrete I/O- en EtherCAT-mapping zijn niet in de PLC-code aangetroffen. Er is dus geen zichtbare I/O-mapping in GVL_Inputs of GVL_Outputs. Zie sectie 2 voor details.

## 2. Systeemconfiguratie

### I/O-mapping
In de huidige projectbestanden zijn geen fysieke I/O-variabelen gedefinieerd:
- [GVL_Inputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Inputs.TcGVL)
- [GVL_Outputs.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_Outputs.TcGVL)

De transportband wordt functioneel gesimuleerd in [FB_Transportband.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU) met een timer. Daardoor is de werking deterministisch binnen de PLC-cyclus en eenvoudig te vervangen door echte I/O.

Logische I/O in dit project:
- VISU-inputs sturen procescommando's naar de PLC.
- MQTT-topics vormen de logische I/O naar externe subsystemen, zoals robotarm, transportband en mixer.

### EtherCAT-apparaten
In de PLC-bronbestanden zijn geen specifieke EtherCAT-I/O-apparaten of mappings gevonden. Eventuele EtherCAT-configuratie staat buiten de PLC-code, in de TwinCAT System Manager-configuratie, die niet in de repository zichtbaar is.

### Taakinstellingen en cyclustijden
In de PLC-POU's zijn geen expliciete cyclustijden of taakconfiguraties gedeclareerd. Die staan normaal in de TwinCAT-projectconfiguratie. In de repository is alleen de PLC-code opgenomen.

Conceptuele timing uit de code:
- De PLC-cyclus bepaalt de resolutie van commando-pulsen via rising-edge-detectie met R_TRIG in MAIN.
- MQTT-publiceren en -abonneren wordt per PLC-cyclus afgehandeld in FB_MQTT, met een interne message queue.
- De transportbandsimulatie gebruikt een TON met een looptijd van 3 seconden.
- De heartbeat-watchdog van de robot werkt met een timeout van 12 seconden en wordt per cyclus bijgewerkt.

## 3. MQTT-integratie

### 3.1 Broker en topics

Broker en topics staan in [GVL_MQTT.TcGVL](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/GVLs/GVL_MQTT.TcGVL):

```text
sMqttBroker := '192.168.1.240'
nMqttPort := 1883

sTopicRobotCmd := 'pancake/robot/cmd'
sTopicRobotState := 'pancake/robot/state'
sTopicRobotError := 'pancake/robot/error'
sTopicRobotHeartbeat := 'pancake/robot/heartbeat'

sTopicTransportbandState := 'pancake/transportband/state'
```

### 3.2 Commando-payloads

```json
{ "cmd": "teach", "pose": "camera" }
{ "cmd": "goto",  "pose": "scale" }
{ "cmd": "save" }
{ "cmd": "stop" }
```

### 3.3 Status- en foutpayloads

Robotstatus:
```json
{ "state": "idle" }
{ "state": "running" }
{ "state": "done" }
{ "state": "at_camera" }
{ "state": "teach_camera" }
```

Robotfout:
```json
{ "state": "ok",    "error": false, "message": "" }
{ "state": "error", "error": true,  "message": "unknown command" }
```

### 3.4 Publicatie- en abonnementsflow

- FB_MQTT configureert de MQTT-client en abonneert zich op robot-, transportband- en mixer-topics.
- Robotcommando's worden gepubliceerd bij een VISU-trigger met QoS 1.
- Transportbandstatus wordt gepubliceerd bij een statuswijziging met QoS 0.
- Robotstatus en foutstatus worden geparsed en vertaald naar PLC-flags.

Flow in woorden:
- VISU → MAIN zet commandoflags → FB_MQTT publiceert JSON.
- Robotarm publiceert state, error en heartbeat → FB_MQTT parseert → GVL_MQTT-status wordt bijgewerkt.
- Transportbandstatus wordt gepubliceerd zolang de transportband actief of klaar is.

## MODULE 1: MAIN - Centrale stuurlogica en commandoverwerking

### 1.1 Doel van de bouwsteen

MAIN is het centrale orchestratiehart van het PLC-systeem. Het vertaalt menselijke input via VISU-knoppen en automatische signalen naar commando-pulsen voor de state machine, transportband en robotarm. MAIN integreert de functionblokken FB_StateMachine, FB_Transportband, FB_MQTT en FB_QrVision en beheert de globale status voor visualisatie.

Toepassingscontext:
- Automatisering van het pannenkoekenproces
- Centraal distributiepunt voor commando's
- VISU-interface voor bediening en statusweergave

Bestand: [MAIN.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/MAIN.TcPOU)

### 1.2 I/O- en variabelentabel

| Variabelenaam | Type (In/Out/Intern) | Datatype | Functie |
|---|---|---|---|
| `fbStateMachine` | Intern | FB_StateMachine | State-machine-instantie voor proceslogica |
| `fbTransportband` | Intern | FB_Transportband | Motorbesturing van de transportband |
| `fbMqtt` | Intern | FB_MQTT | MQTT-communicatieclient |
| `fbQrVision` | Intern | FB_QrVision | QR-code detectie-engine |
| `rVisuStartTap` | Intern | R_TRIG | Rising-edge detector voor de START-knop |
| `rVisuStopTap` | Intern | R_TRIG | Rising-edge detector voor de STOP-knop |
| `rRobotTeachTap` | Intern | R_TRIG | Rising-edge detector voor de TEACH-knop |
| `rRobotGotoTap` | Intern | R_TRIG | Rising-edge detector voor de GOTO-knop |
| `rRobotSaveTap` | Intern | R_TRIG | Rising-edge detector voor de SAVE-knop |
| `rRobotStopTap` | Intern | R_TRIG | Rising-edge detector voor de STOP ROBOT-knop |
| `bStartCmd` | Intern | BOOL | Puls naar state machine: start proces |
| `bStopCmd` | Intern | BOOL | Puls naar state machine: stop proces |
| `bRobotTeachCmd` | Intern | BOOL | Puls naar FB_MQTT: robot teach-commando |
| `bRobotGotoCmd` | Intern | BOOL | Puls naar FB_MQTT: robot goto-commando |
| `bRobotSaveCmd` | Intern | BOOL | Puls naar FB_MQTT: robot save-commando |
| `bRobotStopCmd` | Intern | BOOL | Puls naar FB_MQTT: robot stop-commando |
| `bInductiveSensor` | In | BOOL | Digitale ingang: inductieve sensor van de transportband |
| `nMotorOutput` | Out | INT | Analoge uitgang: DC-motorstuurspanning (0-32767 = 0-10V) |
| `GVL_MQTT.bVisuStartTransportband` | In (Global) | BOOL | VISU: transportband starten |
| `GVL_MQTT.bVisuStopTransportband` | In (Global) | BOOL | VISU: transportband stoppen |
| `GVL_MQTT.bVisuStartSequence` | In (Global) | BOOL | VISU: automatische sequentie starten |
| `GVL_MQTT.bVisuAutoSequenceEnable` | In/Out (Global) | BOOL | VISU: automatisering in- of uitschakelen |
| `GVL_MQTT.bRobotArmOnline` | Out (Global) | BOOL | Status: robotarm is online |
| `GVL_MQTT.bRobotArmRunning` | Out (Global) | BOOL | Status: robotarm is actief |
| `GVL_MQTT.bRobotArmBusy` | Out (Global) | BOOL | Status: robotarm is bezig |
| `GVL_MQTT.bRobotArmDone` | Out (Global) | BOOL | Status: robotarm is klaar |
| `GVL_MQTT.bRobotArmTeachMode` | Out (Global) | BOOL | Status: robotarm staat in teach-mode |
| `GVL_MQTT.bRobotArmAlarm` | Out (Global) | BOOL | Status: robotarm-alarm actief |
| `GVL_MQTT.sRobotArmDisplay` | Out (Global) | STRING | Displaytekst voor robotstatus |
| `GVL_MQTT.sTransportbandDisplay` | Out (Global) | STRING | Displaytekst voor transportbandstatus |
| `GVL_MQTT.sCurrentState` | Out (Global) | STRING | Displaytekst voor de huidige state-machine state |
| `GVL_Data.bQrCodeFound` | Out (Global) | BOOL | Status: QR-code gedetecteerd |
| `GVL_Data.sQrCodeText` | Out (Global) | STRING | QR-code-inhoud in ruwe tekst |
| `GVL_Data.sQrDisplayText` | Out (Global) | STRING | QR-displaytekst met overlay-informatie |

### 1.3 Functionele werking stap voor stap

#### Fase 1: VISU-knophandler voor puls-generatie

1. R_TRIG-detectie van de START-knop:
   - `rVisuStartTap` detecteert de stijgende flank van `GVL_MQTT.bVisuStartTransportband` of `GVL_MQTT.bVisuStartSequence`.
   - Als de automatische sequentie-start wordt ingedrukt, wordt `GVL_Config.bAutoSequenceEnable = TRUE` gezet en is automatisering actief.
   - `bStartRequest` wordt `rVisuStartTap.Q` of `fbMqtt.bRemoteStart`.

2. R_TRIG-detectie van de STOP-knop:
   - `rVisuStopTap` detecteert de stijgende flank van `GVL_MQTT.bVisuStopTransportband`.
   - `bStopRequest` wordt `rVisuStopTap.Q` of `fbMqtt.bRemoteStop`.

3. Robotarmcommando's wanneer automatisering uit staat:
   - Dit is alleen actief als `GVL_Config.bAutoSequenceEnable = FALSE`.
   - Elke R_TRIG voor teach, goto, save en stop detecteert knop-pulsen.
   - Prioriteit via IF-ELSIF-cascade:
     - STOP → `bRobotStopCmd = TRUE`.
     - SAVE → `bRobotSaveCmd = TRUE`.
     - TEACH → `bRobotTeachCmd = TRUE`.
     - GOTO → `bRobotGotoCmd = TRUE`.

4. VISU-aanvraagflags wissen:
   - Na elke cyclus worden alle VISU-inputflags gewist: teach, goto, save, stop en sequence.
   - Daardoor accumuleren knop-pulsen niet en triggert elke klik exact één commando.

#### Fase 2: Automatische procesflow via de state machine

5. Aansturen van de state machine:
   - `fbStateMachine()` wordt aangeroepen met parameters zoals automatisering, start- en stopverzoeken, feedbacksignalen, pose-strings en timeout-instellingen.

6. Commando's uit de state machine halen:
   - `bStartCmd` zet de transportband in beweging.
   - `bStopCmd` stopt de transportband.
   - `bRobotGotoCmd` stuurt de robotarm naar de volgende pose.
   - `bRobotStopCmd` geeft een noodstop aan de robot.
   - TEACH- en SAVE-commando's worden in automatisering niet uitgevoerd.

#### Fase 3: Robotstatus-updates van MQTT naar display

7. Robotstatus mappen:
   - De statusflags in GVL_MQTT worden gevuld vanuit FB_MQTT.

8. Robot-displaytekst genereren:
   - Offline → "Robot Arm OFFLINE".
   - Alarm → "Robot Arm ALARM".
   - Teach-mode → "Robot Arm TEACH".
   - Busy → "Robot Arm BUSY".
   - Klaar → "Robot Arm READY".

#### Fase 4: Transportbandbesturing

9. Transportband aanroepen:
   - `fbTransportband(bStart := bStartCmd, bStop := bStopCmd, tDuration := GVL_Config.tFeedDuration)`.
   - De timer start bij de rising edge van `bStartCmd`.
   - De looptijd wordt bepaald door `tFeedDuration`, meestal 3 seconden.

10. Transportbandstatus bijwerken:
    - `GVL_MQTT.bTransportbandRunning` komt van `fbTransportband.bRunning`.
    - `GVL_MQTT.bTransportbandDone` komt van `fbTransportband.bDone`.
    - `nMotorOutput` komt van `fbTransportband.nMotorControl`.

11. Transportbanddisplay:
    - Actief → "Transportband ON".
    - Cyclus voltooid → "Transportband READY".
    - Anders → "Transportband OFF".

#### Fase 5: MQTT en QR Vision

12. MQTT-publicatie in FB_MQTT:
    - Robotcommando's worden gepubliceerd zodra `bRobotXxxCmd`-pulsen binnenkomen.
    - De transportbandstatus wordt continu gepubliceerd.

13. QR Vision:
    - `fbQrVision(bEnable := GVL_Data.bQrEnabled OR fbStateMachine.bQrAutoEnable)`.
   - QR-detectie wordt ingeschakeld via een VISU-schakelaar of automatisch via de state machine.
    - `GVL_Data.bQrCodeFound` komt van `fbQrVision.bCodeFound`.
    - `GVL_Data.sQrCodeText` komt van `fbQrVision.sCodeAsString`.

#### Fase 6: Display en monitoring

14. State-string output:
    - `GVL_MQTT.sCurrentState` krijgt `TO_STRING(fbStateMachine.eStateOut)` als leesbare statustekst.

15. Publicatie naar HMI:
    - Alle displaystrings en statusflags zijn daarna beschikbaar in GVL_MQTT voor de VISU.

### 1.4 Veiligheid en interlocks

| Veiligheidsvoorwaarde | Implementatie | Effect |
|---|---|---|
| Robotarm-stop heeft prioriteit | Stop wordt als eerste gecontroleerd | De noodstop van de robot werkt onmiddellijk |
| Automatiseringsschakelaar | `bAutoSequenceEnable` schakelt automatische commando's uit | De operator kan overschakelen naar handmatig |
| Transportband-noodstop | `bStop` reset de cyclus en de motor | De motor stopt onmiddellijk |
| Isolatie van commando-pulsen | R_TRIG zorgt voor flankdetectie in plaats van level-triggering | Geen herhalende commando's bij een vastzittende VISU-input |
| Robot-wachtlogica | De state machine wacht op `bRobotDone` of een timeout van 12 seconden | De timeout voorkomt oneindig wachten |
| VISU-inputs wissen | Alle VISU-flags worden na elke cyclus gewist | Geen vastzittende commando's van een vorige cyclus |

Afwezige safety-functies die extern geborgd moeten zijn:
- Fysieke noodstop of veilige I/O
- Safe Torque Off voor de robotmotor
- Overloadbeveiliging voor de transportbandmotor
- Mechanische blokkeringen en interlocks voor de robotarm

### 1.5 Foutafhandeling en alarmen

| Foutconditie | Detectie | Reactie | Resetmechanisme |
|---|---|---|---|
| Robot offline | Heartbeat-timeout: 12 seconden geen `pancake/robot/heartbeat` | `bRobotArmOnline = FALSE`, display "OFFLINE" | Automatisch zodra heartbeat terugkomt |
| Robotalarm | MQTT-foutpayload parsing in FB_MQTT | `bRobotArmAlarm = TRUE`, display "ALARM" | Robot stuurt weer `ok` of een gezonde status |
| Transportband-timeout | Niet expliciet geïmplementeerd; timer staat op 3 seconden | Timer loopt vanzelf af | Cyclus is daarna voltooid |
| QR niet gevonden | `fbQrVision.bCodeFound = FALSE` na timeout van 2 seconden | `sQrDisplayText = "No QR Code Found"`, processtap wordt overgeslagen | Volgende cyclus opnieuw proberen |
| MQTT-broker down | `fbMqttClient.bConnected = FALSE` | Subscriptions worden gereset en statusflags blijven staan | Automatisch bij reconnect |
| Systeemfoutflag | `GVL_MQTT.bSystemError` | State machine gaat naar ERROR | Operator drukt STOP |

## MODULE 2: FB_StateMachine - Automatische procesflow en robotarmorkestratie

### 2.1 Doel van de bouwsteen

FB_StateMachine implementeert de volledig geautomatiseerde procesflow. De bouwsteen beheert:
- Sequentiële stappen zoals transportband, camera, schaal, mixer, inductieve sensor en afval
- Tijdsbepaling van robotcommando's zoals goto, wachten en bevestiging
- Timeout- en foutafhandeling
- QR-code scanvenster

Toepassingscontext:
- Automatisering van pannenkoekenproductie
- Meerstaps robotorkestratie met sensorkoppeling
- Realtime timingbesturing met timers

Bestand: [FB_StateMachine.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_StateMachine.TcPOU)

### 2.2 I/O- en variabelentabel

| Variabelenaam | Type (In/Out/Intern) | Datatype | Functie |
|---|---|---|---|
| `bAutoEnable` | In | BOOL | Is de automatisering actief? |
| `bStartRequest` | In | BOOL | Start van de automatische sequentie |
| `bStopRequest` | In | BOOL | Stop of reset |
| `bError` | In | BOOL | Systeemfout actief? |
| `bTransportbandDone` | In | BOOL | Transportbandcyclus voltooid |
| `bRobotBusy` | In | BOOL | Robot bezig met beweging |
| `bRobotDone` | In | BOOL | Robot heeft het target bereikt |
| `bQrCodeFound` | In | BOOL | QR-code gedetecteerd |
| `bInductiveSensor` | In | BOOL | Inductieve sensor actief, dus pannenkoek aanwezig |
| `tRobotTimeout` | In | TIME | Timeout per robotstap, standaard 12 seconden |
| `tQrScanDuration` | In | TIME | QR-scanvenster, standaard 2 seconden |
| `tSensorWaitDuration` | In | TIME | Wachttijd voor de inductieve sensor, standaard 3 seconden |
| `sPoseCamera` | In | STRING | Robotpose voor camera-opname |
| `sPoseScale` | In | STRING | Robotpose voor weging op schaal |
| `sPoseMixer` | In | STRING | Robotpose voor de mixer |
| `sPoseInductive` | In | STRING | Robotpose boven de inductieve sensor |
| `sPoseDiscard` | In | STRING | Robotpose voor de afvalcontainer |
| `eStateOut` | Out | E_MainState | Huidige state voor debug en display |
| `bStartCmd` | Out | BOOL | Commando: start transportband |
| `bStopCmd` | Out | BOOL | Commando: stop transportband |
| `bRobotGotoCmd` | Out | BOOL | Commando: robotarm naar een pose |
| `bRobotStopCmd` | Out | BOOL | Commando: robot-noodstop |
| `bQrAutoEnable` | Out | BOOL | Enabler voor het QR-scanvenster |
| `sRobotTargetPose` | Out | STRING | Doelpose voor de huidige robotstap |
| `eState` | Intern | E_MainState | Huidige interne state |
| `bRobotBusySeen` | Intern | BOOL | Flag dat de robot in deze stap busy is geweest |
| `tRobotTimeoutFb` | Intern | TON | Timer voor robotrespons-timeout |
| `tQrScanTimerFb` | Intern | TON | Timer voor de QR-scanduur |
| `tSensorWaitTimerFb` | Intern | TON | Timer voor de sensorduur |

### 2.3 Functionele werking stap voor stap

1. De state machine start in `INIT`.
2. De eerste cyclus gaat direct naar `IDLE` zonder actieve commando's.
3. Alle timers en outputs zijn FALSE of 0.
4. In `IDLE` wordt op start gewacht.
5. Als `bError = TRUE`, dan gaat de state machine naar `ERROR`.
6. Anders, als `bAutoEnable = TRUE` en `bStartRequest = TRUE`, dan start de sequentie en gaat de state machine naar `SEQ_FEED_BLOCK`.
7. Tegelijk wordt `bRobotBusySeen = FALSE` gereset voor een schone start van de volgende beweging.
8. In `SEQ_FEED_BLOCK` zet `bStartCmd := TRUE` de transportband aan.
9. Als `bStopRequest` of `bError` actief wordt, gaat de state machine naar `SEQ_ERROR` voor een gecontroleerde afbreking.
10. Als `bTransportbandDone` actief wordt, gaat de state machine naar `SEQ_MOVE_CAMERA`.
11. In `SEQ_MOVE_CAMERA` wordt `bRobotGotoCmd := TRUE` gezet en `sRobotTargetPose := sPoseCamera`.
12. Daarna gaat de state machine direct naar `SEQ_WAIT_CAMERA`.
13. In `SEQ_WAIT_CAMERA` bewaakt `tRobotTimeoutFb` de 12-seconden-timeout.
14. Als `bRobotBusy` actief wordt, zet de state machine `bRobotBusySeen := TRUE`.
15. Als `bRobotBusySeen` en `bRobotDone` waar zijn en `bRobotBusy` niet meer actief is, dan is de robot aangekomen en gaat de state machine naar `SEQ_SCAN_CAMERA`.
16. Als de timeout afloopt, gaat de state machine naar `SEQ_ERROR`.
17. In `SEQ_SCAN_CAMERA` activeert `bQrAutoEnable := TRUE` de QR-visiondecoder.
18. `tQrScanTimerFb` bewaakt het scanvenster van 2 seconden.
19. Als `bQrCodeFound = TRUE`, dan gaat de state machine naar `SEQ_MOVE_SCALE`.
20. Als de timer afloopt zonder QR, dan gaat de state machine alsnog door naar `SEQ_MOVE_SCALE`.
21. In `SEQ_MOVE_SCALE` wordt `bRobotGotoCmd := TRUE` gezet en `sRobotTargetPose := sPoseScale`.
22. Daarna gaat de state machine naar `SEQ_WAIT_SCALE`.
23. In `SEQ_WAIT_SCALE` geldt dezelfde logica als bij camera.
24. Bij succes gaat de state machine naar `SEQ_MOVE_MIXER`.
25. In `SEQ_MOVE_MIXER` wordt `bRobotGotoCmd := TRUE` gezet en `sRobotTargetPose := sPoseMixer`.
26. Daarna gaat de state machine naar `SEQ_WAIT_MIXER`.
27. In `SEQ_WAIT_MIXER` geldt dezelfde wachtlogica.
28. Bij succes gaat de state machine naar `SEQ_MOVE_INDUCTIVE`.
29. In `SEQ_MOVE_INDUCTIVE` wordt `bRobotGotoCmd := TRUE` gezet en `sRobotTargetPose := sPoseInductive`.
30. Daarna gaat de state machine naar `SEQ_WAIT_INDUCTIVE`.
31. In `SEQ_WAIT_INDUCTIVE` is dezelfde timeout-, busy- en done-detectie actief.
32. Als de robot is aangekomen en `bInductiveSensor = TRUE`, dan is de pannenkoek gedetecteerd en gaat de state machine naar `SEQ_MOVE_DISCARD`.
33. Als de robot is aangekomen maar de sensor nog niet actief is, dan start `tSensorWaitTimerFb`.
34. Als de sensor alsnog triggert of de timer afloopt, gaat de state machine naar `SEQ_MOVE_DISCARD`.
35. In `SEQ_MOVE_DISCARD` wordt `bRobotGotoCmd := TRUE` gezet en `sRobotTargetPose := sPoseDiscard`.
36. Daarna gaat de state machine naar `SEQ_WAIT_DISCARD`.
37. In `SEQ_WAIT_DISCARD` geldt dezelfde wachtlogica als bij de vorige robotstappen.
38. Bij succes gaat de state machine naar `SEQ_COMPLETE`.
39. In `SEQ_COMPLETE` worden alle outputs gereset naar FALSE.
40. De state machine gaat terug naar `IDLE` en wacht op een volgende startpuls.
41. In `SEQ_ERROR` zet `bRobotStopCmd := TRUE` de robot stil.
42. Alle timers en commando's worden gereset.
43. Daarna gaat de state machine terug naar `IDLE`, zodat herstel automatisch mogelijk is.
44. In `ERROR` heeft `bError = TRUE` voorrang op alles.
45. De state machine keert alleen terug naar `IDLE` als `bError` is opgeheven.

### 2.4 Veiligheid en interlocks

| Veiligheidsvoorwaarde | Implementatie | Effect |
|---|---|---|
| Robottimeout per stap | TON-timer van 12 seconden met automatische abort bij timeout | Voorkomt vastlopen bij robotcommunicatiefouten |
| Robot busy-done-handshake | `bRobotBusySeen` zorgt voor flankgestuurde detectie | Flicker in robotstatus wordt genegeerd |
| Gecontroleerde sensorfail | QR-timeout van 2 seconden en sensortimeout van 3 seconden, daarna doorgaan | Geen harde blokkade bij sensorfalen |
| Directe stoprequest-afhandeling | In elke stap wordt op `bStopRequest` gecontroleerd | Noodstop werkt onmiddellijk |
| Isolatie van fouttoestand | Fouttoestand leidt naar `SEQ_ERROR`, daarna robot stoppen en terug naar idle | Voorkomt kettingreacties van fouten |

### 2.5 Foutafhandeling en alarmen

| Foutconditie | Detectie | Reactie | Resetmechanisme |
|---|---|---|---|
| Robottimeout | TON verloopt na 12 seconden per stap | Naar `SEQ_ERROR`, robot stoppen | Operator start opnieuw via `bStartRequest` |
| QR niet gedetecteerd | Timer verloopt in `SEQ_SCAN_CAMERA` | Toch doorgaan naar de volgende stap | Wordt gelogd in het QR-blok |
| Sensortimeout | Timer verloopt in `SEQ_WAIT_INDUCTIVE` | Toch doorgaan naar de afvalstap | Operator kan opnieuw proberen |
| Systeemfout | `bError = TRUE` | Fouttoestand, robot stop, terug naar `IDLE` | Foutflag wissen |
| Stoprequest tijdens sequentie | `bStopRequest` in een willekeurige stap | Direct naar `SEQ_ERROR` | Operator start opnieuw |

## MODULE 3: FB_Transportband - DC-motorbesturing en timinglogica

### 3.1 Doel van de bouwsteen

FB_Transportband implementeert een eenvoudige maar robuuste motorbesturing voor de transportband. De bouwsteen verzorgt:
- Rising-edge-gestarte motorstart, zodat korte pulsen toch een volledige cyclus activeren
- Deterministische looptijd op basis van een timer
- Noodstop met onmiddellijke uitschakeling van de motor
- Een simulatorvriendelijk ontwerp dat later vervangen kan worden door echte I/O

Toepassingscontext:
- Transport van pannenkoekenblokken via de transportband
- Precisietiming voor processtappen
- EL7332 DC-motor-analoge uitgang van 0-32767 voor 0-10V

Bestand: [FB_Transportband.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_Transportband.TcPOU)

### 3.2 I/O- en variabelentabel

| Variabelenaam | Type (In/Out/Intern) | Datatype | Functie |
|---|---|---|---|
| `bStart` | In | BOOL | Startcommando, flankgestuurd |
| `bStop` | In | BOOL | Stopcommando of noodstop |
| `tDuration` | In | TIME | Looptijd van de motor, standaard 3 seconden |
| `bRunning` | Out | BOOL | Motor actief |
| `bDone` | Out | BOOL | Cyclus voltooid |
| `nMotorControl` | Out | INT | Analoge uitgang 0-32767 voor 0-10V op EL7332 |
| `tRun` | Intern | TON | Timer voor de motorrun |
| `rStartTrig` | Intern | R_TRIG | Rising-edge detector voor de startpuls |
| `bCycleActive` | Intern | BOOL | Latch dat de cyclus actief is |

### 3.3 Functionele werking stap voor stap

1. Als `bStop = TRUE`, dan wordt alles gereset en stopt de motor direct.
2. `rStartTrig(CLK := bStart)` detecteert de overgang van 0 naar 1.
3. Als `rStartTrig.Q = TRUE`, dan wordt `bCycleActive := TRUE` gezet en `nMotorControl := 32767`.
4. Als `bCycleActive = TRUE`, dan draait de motor en wordt `tRun(IN := TRUE, PT := tDuration)` geactiveerd.
5. Als `tRun.Q = TRUE`, dan wordt de cyclus beëindigd, `bDone := TRUE` gezet en de motor uitgeschakeld.
6. Als geen cyclus actief is, staat de motor uit en is de timer uitgeschakeld.

### 3.4 Veiligheid en interlocks

| Veiligheidsvoorwaarde | Implementatie | Effect |
|---|---|---|
| Noodstop heeft prioriteit | `bStop` wordt eerst gecontroleerd en reset alles | Motor stopt onmiddellijk zonder afremming |
| Flankgestarte start | `rStartTrig` detecteert alleen de rising edge | Een korte puls activeert toch een volledige run |
| Latch voorkomt retrigger | `bCycleActive` voorkomt meerdere timerstarts | Eén startpuls levert exact één getimede cyclus op |
| Directe motoruitschakeling | `nMotorControl := 0` wordt meteen gezet | Geen uitrol, directe stop |
| Timer wordt uitgeschakeld na afloop | `tRun(IN := FALSE)` na voltooiing | Timer stapelt niet op en blijft niet hangen |

Ontwerpfilosofie: eenvoudig, deterministisch en fail-safe. Geen PID, geen ramp, geen variabele snelheid, alleen aan of uit.

### 3.5 Foutafhandeling en alarmen

| Foutconditie | Detectie | Reactie | Resetmechanisme |
|---|---|---|---|
| Vastzittende startinput | `bStart` blijft continu hoog | De timer loopt precies één keer per rising edge | De volgende dalende en stijgende flank start een nieuwe cyclus |
| Timerstoring | Door TwinCAT-runtime gedetecteerd | De TON geeft FALSE als de timer niet werkt | Automatisch hersteld door de runtime |
| Ontbrekend stopcommando | Timer wordt extern niet gereset | Timer loopt vanzelf af na `tDuration` | Automatische reset in de code |
| Geen motorfeedback | Er is geen encoderfeedback geïmplementeerd | Alleen de opdrachtstatus is zichtbaar, niet de echte motorstatus | Visuele inspectie of extra feedbacksensor toevoegen |

## MODULE 4: FB_MQTT - Publiceren, abonneren en robotarmcommando's

### 4.1 Doel van de bouwsteen

FB_MQTT implementeert de volledige MQTT-clientlogica voor het pannenkoekensysteem:
- MQTT-brokerverbinding en abonnementsbeheer
- Publicatie van robotarmcommando's zoals teach, goto, save en stop als JSON
- Parsing van robotstatus, foutstatus en heartbeat met statusflag-updates
- Publicatie van transportbandstatus
- Beheer van de message queue en serialisatie van payloads

Toepassingscontext:
- Communicatie tussen PLC en externe robotarm, bijvoorbeeld een Python- of ROS-apparaat
- HMI-statusaggregatie en remote command ingestion
- Heartbeat-watchdog voor online-detectie van de robotarm

Bestand: [FB_MQTT.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_MQTT.TcPOU)

### 4.2 I/O- en variabelentabel

| Variabelenaam | Type (In/Out/Intern) | Datatype | Functie |
|---|---|---|---|
| `bTransportbandRunning` | In | BOOL | Transportbandmotor actief |
| `bTransportbandDone` | In | BOOL | Transportbandcyclus voltooid |
| `bRobotTeachCmd` | In | BOOL | Puls: stuur teach-commando |
| `bRobotGotoCmd` | In | BOOL | Puls: stuur goto-commando |
| `bRobotSaveCmd` | In | BOOL | Puls: stuur save-commando |
| `bRobotStopCmd` | In | BOOL | Puls: stuur stop-commando |
| `sRobotTargetPose` | In | STRING | Doelpose, zoals camera, scale of mixer |
| `bSystemError` | In | BOOL | Systeemfoutstatus |
| `sErrorMessage` | In | STRING | Tekst van de foutmelding |
| `bRemoteStart` | Out | BOOL | Remote startverzoek |
| `bRemoteStop` | Out | BOOL | Remote stopverzoek |
| `bRemoteRobotTeach` | Out | BOOL | Remote teachverzoek voor robot |
| `bRemoteRobotGoto` | Out | BOOL | Remote gotoverzoek voor robot |
| `bRemoteRobotSave` | Out | BOOL | Remote saveverzoek voor robot |
| `bRemoteRobotStop` | Out | BOOL | Remote stopverzoek voor robot |
| `bRobotArmRunning` | Out | BOOL | Robotarm voert momenteel een opdracht uit |
| `bRobotArmDone` | Out | BOOL | Robotarm heeft het target bereikt |
| `bRobotArmOnline` | Out | BOOL | Robotarm is online via heartbeat |
| `bRobotArmBusy` | Out | BOOL | Robotarm is in beweging of bezig |
| `bRobotArmTeachMode` | Out | BOOL | Robotarm staat in teach-mode |
| `bRobotArmAlarm` | Out | BOOL | Robotarm heeft een foutstatus |
| `bRobotArmAckOk` | Out | BOOL | Er is een command acknowledge ontvangen |
| `fbMqttClient` | Intern | FB_IotMqttClient | MQTT-clientinstantie |
| `fbMessageQueue` | Intern | FB_IotMqttMessageQueue | Message-queuebuffer |
| `fbMessage` | Intern | FB_IotMqttMessage | Enkelvoudige message-instantie |
| `bMqttConfigured` | Intern | BOOL | Configuratie slechts één keer uitvoeren |
| `bSubscribedXxx` | Intern | BOOL | Subscriptiestatus per topic |
| `sPayloadRcv`, `sPayloadPub` | Intern | STRING | Buffers voor ontvangen en te publiceren payloads |
| `fbRobotHeartbeatWatchdog` | Intern | TON | Timer voor heartbeat-timeout van 12 seconden |
| `bRobotHeartbeatPulse` | Intern | BOOL | Heartbeat ontvangen in deze cyclus |
| `bRobotStateReceived` | Intern | BOOL | Statebericht ontvangen |
| `bRobotErrorActive` | Intern | BOOL | Actieve foutstatus |

### 4.3 Functionele werking stap voor stap

1. Bij de eerste uitvoering worden broker, poort en client-ID geladen uit GVL_MQTT en wordt `bMqttConfigured := TRUE` gezet.
2. `fbMqttClient.Execute(bConnect := GVL_MQTT.bEnableMqtt)` verzorgt de clientlevenscyclus.
3. Als de client verbonden is, worden de relevante topics met QoS 0 geabonneerd. De `bSubscribedXxx`-flags zorgen dat dit slechts één keer per verbinding gebeurt.
4. Als `bRobotTeachCmd = TRUE`, dan wordt `{"cmd":"teach","pose":"..."}` gepubliceerd op `pancake/robot/cmd` met QoS 1.
5. Als `bRobotGotoCmd = TRUE`, dan wordt `{"cmd":"goto","pose":"..."}` gepubliceerd op `pancake/robot/cmd` met QoS 1.
6. Als `bRobotSaveCmd = TRUE`, dan wordt `{"cmd":"save"}` gepubliceerd op `pancake/robot/cmd` met QoS 1.
7. Als `bRobotStopCmd = TRUE`, dan wordt `{"cmd":"stop"}` gepubliceerd op `pancake/robot/cmd` met QoS 1.
8. De transportbandstatus wordt alleen opnieuw gepubliceerd als die veranderd is. Mogelijke statusberichten zijn `running`, `done` en `idle`.
9. Berichten uit de message queue worden per topic verwerkt. Robotstatus, foutstatus, heartbeat en transportbandstatus krijgen elk hun eigen verwerking.
10. De heartbeat-watchdog gebruikt `fbRobotHeartbeatWatchdog(IN := bRobotHeartbeatPulse, PT := T#12S)`.
11. Als 12 seconden geen heartbeat binnenkomt, wordt `bRobotArmOnline := FALSE`; anders staat de robot online.

### 4.4 Veiligheid en interlocks

| Veiligheidsvoorwaarde | Implementatie | Effect |
|---|---|---|
| Heartbeat-watchdog | Timeout van 12 seconden en automatische offline-detectie | Voorkomt vastlopende commando's als de robot wegvalt |
| QoS 1 voor commando's | Herpublicatie tot de broker bevestigt | Commando's worden betrouwbaar afgeleverd |
| QoS 0 voor status | Lichtgewicht statusupdates | Geen blokkade op statuspublicatie |
| Message queue | Berichten worden sequentieel geparsed | Minder kans op berichtverlies |
| Payloadvalidatie | JSON-structuur wordt impliciet gevalideerd tijdens parsing | Onjuiste JSON wordt overgeslagen |
| Flankgestuurde commando's | Publicatie alleen bij `bRobotXxxCmd = TRUE` | Geen herhaling van commando's |

### 4.5 Foutafhandeling en alarmen

| Foutconditie | Detectie | Reactie | Resetmechanisme |
|---|---|---|---|
| Robot offline | Geen heartbeat gedurende 12 seconden | `bRobotArmOnline := FALSE` | Automatisch zodra heartbeat terugkomt |
| Robotalarm | Errorpayload met `"error":true` | `bRobotArmAlarm := TRUE`, display "ALARM" | Robot stuurt `error:false` of `state:ok` |
| MQTT-broker down | `fbMqttClient.bConnected = FALSE` | Subscriptions worden gereset en statusflags staan stil | Automatische reconnect zodra broker terug is |
| Ongeldige payload | JSON-parsing mislukt stilzwijgend | Status wordt niet bijgewerkt en vorige waarde blijft staan | Volgende geldige message corrigeert de status |
| Message queue vol | Queue raakt vol | Oudste berichten kunnen worden vervangen | Broker kan via QoS 1 opnieuw leveren |
| Commando niet bevestigd | Robot publiceert geen state of heartbeat | Watchdog-timeout na 12 seconden | Operator kan opnieuw proberen |

## MODULE 5: FB_QrVision - Machine vision en QR-codedetectie

### 5.1 Doel van de bouwsteen

FB_QrVision implementeert QR-code-detectie en visualisatie met behulp van TwinCAT Vision:
- GigE-camera-aansturing en beeldacquisitie
- QR-decodering met meerdere strategieën voor robuustheid
- Live beeldoverlay met gedetecteerde contouren en tekst
- Monitoring van de camera-verbinding
- HRESULT-foutregistratie voor diagnose

Toepassingscontext:
- Identificatie van pannenkoekproducten via QR-code
- Realtime visuele feedback in de HMI
- Robuuste QR-decodering onder verschillende licht- en oriëntatiecondities

Bestand: [FB_QrVision.TcPOU](../plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek/PannenkoekSysteem/POUs/FB_QrVision.TcPOU)

### 5.2 I/O- en variabelentabel

| Variabelenaam | Type (In/Out/Intern) | Datatype | Functie |
|---|---|---|---|
| `bEnable` | In | BOOL | QR-visionverwerking inschakelen |
| `bCodeFound` | Out | BOOL | QR-code gedetecteerd in het huidige frame |
| `sCodeAsString` | Out | STRING | Gedecodeerde QR-tekst in ruwe vorm |
| `sDisplayText` | Out | STRING | Displaytekst met overlay-informatie |
| `hrLast` | Out | HRESULT | Laatste HRESULT voor foutdiagnose |
| `bCameraConnected` | Out | BOOL | Camera is geopend en aan het acquireren |
| `fbCameraControl` | Intern | FB_VN_GevCameraControl | Interface voor de GigE-camera |
| `eState` | Intern | ETcVnCameraState | Staat van de camerafsm |
| `hr` | Intern | HRESULT | Werkend HRESULT |
| `ipImageIn` | Intern | ITcVnImage | Invoerbeeld van de camera |
| `ipImageInDisp` | Intern | ITcVnDisplayableImage | Displaybare wrapper |
| `ipImageRGB` | Intern | ITcVnImage | Naar RGB geconverteerd beeld |
| `ipImageResDisp` | Intern | ITcVnDisplayableImage | Resultaatbeeld voor display |
| `ipCodeDecodedList` | Intern | ITcVnContainer | Lijst met gedecodeerde QR-data |
| `ipCodeContourList` | Intern | ITcVnContainer | Contourlijst voor visualisatie |
| `hrQr` | Intern | HRESULT | Resultaat van de QR-operatie |
| `nQrAttempt` | Intern | USINT | Teller voor QR-decodestrategie, van 1 tot 3 |
| `aColorRed` | Intern | TcVnVector4_LREAL | Rode kleur voor contourtekening |

### 5.3 Functionele werking stap voor stap

1. De huidige camerastaat wordt opgevraagd met `fbCameraControl.GetState()`.
2. Als de staat `OPENED`, `STARTACQUISITION` of `ACQUIRING` is, wordt `bCameraConnected := TRUE`; anders `FALSE`.
3. Als de staat niet `ACQUIRING` is en `bEnable = TRUE`, dan wordt `fbCameraControl.StartAcquisition()` aangeroepen.
4. Als de staat `ERROR` is, dan wordt `fbCameraControl.Reset()` uitgevoerd.
5. In de staat `ACQUIRING` wordt het actuele beeld met `GetCurrentImage()` opgehaald.
6. Bij een geldig beeld wordt het beeld naar een displaybaar formaat gekopieerd.
7. De detectie wordt gereset met `bCodeFound := FALSE` en `sCodeAsString := ''`.
8. Daarna volgen drie decodepogingen met verschillende zoekstrategieën voor normal, flipped en inverted QR-codes.
9. Als een poging slaagt, wordt `bCodeFound := TRUE` en wordt de gedecodeerde tekst naar `sCodeAsString` geëxporteerd.
10. Als contouren beschikbaar zijn, wordt het beeld naar RGB geconverteerd, worden contouren rood getekend en wordt de tekst als overlay toegevoegd.
11. Als geen QR wordt gedetecteerd, dan blijft `bCodeFound := FALSE` en wordt `sDisplayText := 'No QR Code Found'` getoond.
12. `hrLast` houdt het laatste HRESULT bij voor diagnose in de HMI.

### 5.4 Veiligheid en interlocks

| Veiligheidsvoorwaarde | Implementatie | Effect |
|---|---|---|
| Camerafout-herstel | Automatische reset bij `TCVN_CS_ERROR` | De camera blijft niet hangen op een fout |
| Enable/disable-isolatie | `bEnable` stuurt de hele pipeline aan | Vision kan uit zonder systeemuitval |
| Multi-strategy robuustheid | Drie QR-decodepogingen na elkaar | Hogere slagingskans onder wisselende omstandigheden |
| Non-blocking bij geen QR | De flow gaat door zonder QR-data | De state machine blijft niet hangen op QR-timeout |
| HRESULT-tracking | Alle operaties leveren een HRESULT op | Fouten zijn logbaar en zichtbaar in de HMI |

### 5.5 Foutafhandeling en alarmen

| Foutconditie | Detectie | Reactie | Resetmechanisme |
|---|---|---|---|
| Camera niet verbonden | `eState ≠ ACQUIRING` | `bCameraConnected := FALSE`, `sDisplayText := 'Camera offline'` | Automatische herstart bij nieuwe `bEnable`-transitie |
| Beeldophaal-fout | `FAILED(hr)` bij `GetCurrentImage()` | Vorig beeld behouden of "No frame" tonen | Camera probeert automatisch opnieuw |
| QR-decode mislukt | Alle drie pogingen falen | `bCodeFound := FALSE`, `sDisplayText := 'No QR Code Found'` | Volgend frame opnieuw proberen |
| Ongeldige QR | Decodering lukt maar de string is leeg | `sCodeAsString := ''`, `sDisplayText := 'QR: (empty)'` | Volgend geldig frame corrigeert dit |
| Vision-libraryfout | `FAILED(hrLast)` na verwerking | Foutcode beschikbaar in `hrLast` voor de HMI | Operator herstart de vision of controleert de camera |

## Appendix: timing- en cyclusoverzicht

### Nominale timing van de procesflow bij automatisering

```text
Cyclus 0:    IDLE, wachten
Cyclus 1:    START → SEQ_FEED_BLOCK, transportband aan
Cyclus 2-4:  Transportband draait, ongeveer 3 seconden
Cyclus 5:    SEQ_MOVE_CAMERA → SEQ_WAIT_CAMERA
Cyclus 6-n:  Robot beweegt naar camera, typisch 2-5 seconden, maximaal 12 seconden timeout
Cyclus n+1:  SEQ_SCAN_CAMERA, QR-scanvenster van 2 seconden
Cyclus n+3:  SEQ_MOVE_SCALE → SEQ_WAIT_SCALE
Cyclus m:    SEQ_MOVE_MIXER → SEQ_WAIT_MIXER
Cyclus p:    SEQ_MOVE_INDUCTIVE → SEQ_WAIT_INDUCTIVE, inclusief sensorlogica
Cyclus q:    SEQ_MOVE_DISCARD → SEQ_WAIT_DISCARD
Cyclus r:    SEQ_COMPLETE → IDLE, klaar voor de volgende cyclus
```

Totale tijd: ongeveer 30 tot 50 seconden, afhankelijk van transporttijd, vijf robotbewegingen van ongeveer 4 seconden, QR-scan van 2 seconden en marges.

### MQTT-publicatiefrequentie

- Robotcommando's: één publicatie per commandopuls, asynchroon vanuit VISU.
- Transportbandstatus: alleen bij wijziging, meestal drie events per cyclus, namelijk start, done en idle.
- Robotstatus en heartbeat: afkomstig van de externe robot, doorgaans continu rond 1 Hz.

### Configuratieparameters in GVL_Config

| Parameter | Datatype | Typische waarde | Functie |
|---|---|---|---|
| `bAutoSequenceEnable` | BOOL | TRUE | Schakelaar voor automatisering aan of uit |
| `tRobotTimeout` | TIME | T#12S | Timeout per robotarmstap |
| `tQrScanDuration` | TIME | T#2S | QR-scanvenster |
| `tSensorWaitDuration` | TIME | T#3S | Wachttijd voor de inductieve sensor |
| `tFeedDuration` | TIME | T#3S | Looptijd van de transportband |
| `sPoseCamera` | STRING | 'camera' | Robotpositie, extern gemapt |
| `sPoseScale` | STRING | 'scale' | Robotpositie |
| `sPoseMixer` | STRING | 'mixer' | Robotpositie |
| `sPoseInductive` | STRING | 'inductive' | Robotpositie |
| `sPoseDiscard` | STRING | 'discard' | Robotpositie |

Alle parameters kunnen via de VISU worden aangepast zonder hercompilatie van de code.