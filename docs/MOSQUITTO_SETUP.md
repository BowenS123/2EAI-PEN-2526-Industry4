# Mosquitto MQTT Broker – Setup op Windows voor lokaal netwerk

Instructies om Mosquitto als LAN MQTT broker te draaien zodat een PSoC6/ESP/... ermee kunnen verbinden zonder toegang tot het internet.

## Inhoudsopgave

- **Aan de slag**
    1. [Installatie](#1-mosquitto-installeren)
    2. [Configuratie LAN-toegang](#2-mosquitto-configureren-voor-lan-toegang)
    3. [(Her)starten mosquitto](#3-mosquitto-service-herstarten)
  
- **Netwerk & Firewall**
    1. [Controle LAN](#4-controleer-of-broker-luistert-op-lan)
    2. [Firewall configuratie](#5-windows-firewall-openen-voor-poort-1883-powershell)
    3. [Verbinding testen](#6-verbinding-testen-lokaal-op-laptop-command-prompt)
    4. [Optionele beveiliging](#8-optioneel-beveiliging-hardenen)

- **Demo & Devices**
    - [PSoC6 Robotarm](#91-psoc6)
    - [ESP32 Mixer](#92-esp32-mixer)
    - [PLC Transportband](#93-beckoff-plc)

-  **Slot samenvatting**
    - [**Checklist**](#samenvatting-checklist)
    - [**Troubleshooting**](#troubleshooting)


## 1. Mosquitto installeren

1. Download de **Windows installer** van [mosquitto.org/download](https://mosquitto.org/download/).
2. Installeer met standaard opties. Standaard installatiepad: `C:\Program Files\mosquitto\`.
3. De installer maakt een Windows Service aan die automatisch start bij opstarten.

## 2. Mosquitto configureren voor LAN-toegang

Na een standaard installatie luistert Mosquitto **alleen op localhost** (`127.0.0.1`), waardoor externe apparaten niet kunnen verbinden.

### 2a. Open de config file als Administrator

```bash
notepad "C:\Program Files\mosquitto\mosquitto.conf"
```

> Of via Verkenner: ga naar `C:\Program Files\mosquitto\`, klik rechts op `mosquitto.conf` → *Openen als Administrator* via Kladblok.

### 2b. Voeg onderaan het bestand toe

```bash
listener 1883 0.0.0.0
allow_anonymous true
```

| Directive | Uitleg |
|---|---|
| `listener 1883 0.0.0.0` | Luister op poort 1883 op **alle** network interfaces (LAN + localhost) |
| `allow_anonymous true` | Clients mogen verbinden **zonder** gebruikersnaam/wachtwoord |

> **Productie:** vervang `allow_anonymous true` door `password_file` + `acl_file` wanneer de machine buiten een gesloten testnetwerk staat. Zie [Optionele beveiliging](#8-optioneel-beveiliging-hardenen)

### 2c. Sla op en sluit Kladblok



## 3. Mosquitto service herstarten

Open **Command Prompt of PowerShell als Administrator** en voer uit:

```bash
net stop mosquitto
net start mosquitto
```

Verwachte output:
```
The Mosquitto Broker service is stopping.
The Mosquitto Broker service was stopped successfully.

The Mosquitto Broker service was started successfully.
```

---

## 4. Controleer of broker luistert op LAN

```
netstat -an | findstr 1883
```

Verwachte output na correcte configuratie:
```
TCP    0.0.0.0:1883    0.0.0.0:0    LISTENING
```

> Als je alleen `127.0.0.1:1883` ziet, is stap 2 niet goed opgeslagen of is de service niet herstart.


## 5. Windows Firewall openen voor poort 1883 (powershell)

Windows Firewall blokkeert standaard inkomende verbindingen van andere apparaten op het netwerk, ook als de broker wel luistert. Voer onderstaande uit in een **elevated PowerShell** (Als Administrator):

```powershell
New-NetFirewallRule `
    -DisplayName "Mosquitto MQTT 1883" `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 1883 `
    -Action Allow `
    -Profile Any `
    -Enabled True
```

### Controleer of de regel actief is (powershell)

```powershell
Get-NetFirewallRule -DisplayName "Mosquitto MQTT 1883" | Format-Table DisplayName,Enabled,Direction,Action -AutoSize
```

Verwachte output:
```
DisplayName         Enabled Direction Action
-----------         ------- --------- ------
Mosquitto MQTT 1883    True  Inbound  Allow
```


## 6. Verbinding testen (lokaal op laptop) (command prompt)

Gebruik de meegeleverde command-line tools van Mosquitto.

### Verkrijg uw laptop IP (IPv4-Adress)

```bash
ipconfig;
```

### Terminal 1 – Subscribe op alle pancake topics

```bash
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h <laptop-ip> -p 1883 -t "pancake/#" -v
```

### Terminal 2 – Publiceer een testcommando 

```bash
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <laptop-ip> -p 1883 -t "pancake/robot/cmd" -m "{\"cmd\":\"pick\"}" -q 1
```

Terminal 1 moet nu tonen:
```json
pancake/robot/cmd {"cmd":"pick"}
```

Als dit werkt, kan de PSoC6 ook verbinden.


## 8. (Optioneel) Beveiliging hardenen

Alleen relevant buiten een gesloten testnetwerk.

### 8a. Aanmaken password file

```bash
"C:\Program Files\mosquitto\mosquitto_passwd.exe" -c "C:\Program Files\mosquitto\pwfile.txt" psoc6user
```

### 8b. Aanmaken ACL file

Maak `C:\Program Files\mosquitto\aclfile.txt` aan met:

```
user psoc6user
topic readwrite pancake/#
```

### 8c. Config aanpassen

Vervang de twee regels uit stap 2b door:

```bash
listener 1883 0.0.0.0
allow_anonymous false
password_file C:\Program Files\mosquitto\pwfile.txt
acl_file C:\Program Files\mosquitto\aclfile.txt
```

[3. Herstart de Mosquitto service](#3-mosquitto-service-herstarten). Vergeet ook code niet aan te passen.

---

## 9. Demo

## 9.1 PSoC6

### PSoC6 configuratie

1. Verkrijg uw laptop IP (IPv4-Adress)

```bash
ipconfig;
```

2. De PSoC6 broker-instellingen staan in `configs/mqtt_client_config.h`:

```c
#define MQTT_BROKER_ADDRESS    "192.168.0.197"   // IP van je laptop
#define MQTT_PORT              1883
#define MQTT_SECURE_CONNECTION (0)              // Geen TLS vereist
```

3. `configs/wifi_config.h` template:

```c
#ifndef WIFI_CONFIG_H_
#define WIFI_CONFIG_H_
#include "cy_wcm.h"

#define WIFI_SSID                         "Automation" /* SSID of the Wi-Fi Access Point to which the MQTT client connects. */
#define WIFI_PASSWORD                     "Automation" /* Passkey of the Wi-Fi. */
#define WIFI_SECURITY                     CY_WCM_SECURITY_WPA2_AES_PSK /* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure */

#define MAX_WIFI_CONN_RETRIES             (120u) /* Maximum Wi-Fi re-connection limit. */
#define WIFI_CONN_RETRY_INTERVAL_MS       (5000) /* Wi-Fi re-connection time interval in milliseconds. */

#endif /* WIFI_CONFIG_H_ */
```

4. Topics:

| Macro | Topic | QoS | Richting |
|---|---|---|---|
| `MQTT_CMD_TOPIC` | `pancake/robot/cmd` | 1 | PSoC6 subscribes |
| `MQTT_TRANSPORT_STATE_TOPIC` | `pancake/transportband/state` | 0 | PSoC6 subscribes |
| `MQTT_STATE_TOPIC` | `pancake/robot/state` | 0 | PSoC6 publishes |
| `MQTT_ERROR_TOPIC` | `pancake/robot/error` | 1 | PSoC6 publishes |

Na een correcte setup toont de PSoC6 seriële output:
```bash
MQTT connection successful.
MQTT client subscribed to the topic 'pancake/robot/cmd' successfully (QoS 1).
Subscriber: startup -> published idle
```

5. De huidige demo-flow is als volgt:
    1. PSoC6 komt online en publiceert direct `{"state":"idle"}` op `pancake/robot/state`
    2. De transportband publiceert `{"state":"done"}` op `pancake/transportband/state`
    3. De robot publiceert `{"state":"running"}` voor 3 seconden
    4. Daarna publiceert de robot opnieuw `{"state":"idle"}`

De robot kan daarnaast nog steeds reageren op `pancake/robot/cmd`. In de huidige code kan zowel een robot command als een transportband `done` de 3-seconden `running` demo starten.

### Stap 1 — Monitor alle pancake topics

Open terminal & laat deze subscriber open staan:

```bash
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h <ip> -p 1883 -t "pancake/#" -v
```

### Stap 2 — Controle opstartbericht

Na MQTT connectie moet de monitor tonen:

```json
pancake/robot/state {"state":"idle"}
pancake/robot/error {"state":"ok","error":false,"message":""}
```

### Stap 3 — Trigger via transportband

Publiceer een `done` bericht op het transportband topic:

```bash
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <ip> -p 1883 -t "pancake/transportband/state" -m "{\"state\":\"done\"}"
```

Verwachte output in de monitor:

```json
pancake/transportband/state {"state":"done"}
pancake/robot/state {"state":"running"}
pancake/robot/error {"state":"ok","error":false,"message":""}
```

### Stap 4 — Stop trigger via robot command

Je kunt dit stoppen/resetten via een robot command:

```bash
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <ip> -p 1883 -t "pancake/robot/cmd" -m "{\"cmd\":\"stop\"}"
```

### PSoC robotarm samenvatting: Input => Output 

| Trigger | Directe robot state | Na executie process |
|---|---|---|
| Opstarten | `{"state":"idle"}` | blijft `idle` |
| `pancake/transportband/state` = `{"state":"done"}` | `{"state":"running"}` | `{"state":"done"}` |
| `pancake/robot/cmd` = `{"cmd":"start"}` | `{"state":"running"}` | `{"state":"done"}` |
| `pancake/robot/cmd` = `{"cmd":"stop"}` | `{"state":"idle"}` | geen timer 
| Onbekend command | error topic | geen timer |

## 9.2 ESP32 Mixer

### Configuratie
1. In `src/config.h`, pas de Wi-Fi instellingen aan:
```c
#ifndef CONFIG_H
#define CONFIG_H
const char* WIFI_SSID="Automation";
const char* WIFI_PASSWORD="Automation";
#endif
```

2. In `main.cpp`, pas de MQTT instellingen aan:
```c
// MQTT instellingen
const char* mqtt_server = "192.168.1.12";
const int mqtt_port = 1883;
```

### Stap 1: Opstarten subscriber mosquito

```bash
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h <ip> -p 1883 -t "pancake/#" -v
```

Controle: dit is output als succesvol geconnecteerd en opgestart:

```json
Published to pancake/mixer/status: {"state":"connected"}
```

### Stap 2: publisher mosquito

```bash
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <ip> -p 1883 -t "pancake/mixer/cmd" -m "{\"cmd\":\"start\"}"
```

### Error payloads

De robot publiceert momenteel deze error-JSON structuur:

```json
{"state":"ok","error":false,"message":""}
```

Bij een fout, bijvoorbeeld een onbekend command, wordt dat:

```json
{"state":"error","error":true,"message":"unknown command"}
```

## 9.3 Beckoff PLC

### Configuratie & Topics in `GVL_MQTT`:

```iecst
VAR_GLOBAL
	// MQTT Broker Configuratie - HiveMQ Public Broker
	sMqttBroker			: STRING(255) := '192.168.0.197';	// HiveMQ Public Broker (laptop ip)
	nMqttPort			: UINT := 1883;						// Standard MQTT port
	sMqttClientID		: STRING(255) := 'Twincat_Test';    // Unieke client ID
	bEnableMqtt			: BOOL := TRUE;						// VISU: MQTT connect aan/uit
    	
	// MQTT Topics - PSOC: Robotarm
	sTopicRobotCmd				: STRING(255) := 'pancake/robot/cmd';
	sTopicRobotState			: STRING(255) := 'pancake/robot/state';
	sTopicRobotError			: STRING(255) := 'pancake/robot/error';

	// MQTT Topics - PLC: Transportband
	sTopicTransportbandCmd		: STRING(255) := 'pancake/transportband/cmd';
	sTopicTransportbandState	: STRING(255) := 'pancake/transportband/state';
	sTopicTransportbandError	: STRING(255) := 'pancake/transportband/error';

	// MQTT Topics - ESP: Mixer
	sTopicMixerCmd				: STRING(255) := 'pancake/mixer/cmd';
	sTopicMixerState			: STRING(255) := 'pancake/mixer/state';
	sTopicMixerError			: STRING(255) := 'pancake/mixer/error';
    //...
END_VAR
```

### (Opstart)code in `FB_MQTT`:

```iecst
IF NOT bMqttConfigured THEN
	bMqttConfigured := TRUE;
	fbMqttClient.sHostName := GVL_MQTT.sMqttBroker;
	fbMqttClient.nHostPort := GVL_MQTT.nMqttPort;
	fbMqttClient.sClientId := GVL_MQTT.sMqttClientID;
	fbMqttClient.ipMessageQueue := fbMessageQueue;
END_IF

fbMqttClient.Execute(bConnect := GVL_MQTT.bEnableMqtt);
GVL_MQTT.bMqttConnected := fbMqttClient.bConnected;
IF fbMqttClient.bConnected THEN // publish data
    // ...
IF fbMessageQueue.nQueuedMessages > 0 THEN // subscribe data
	IF fbMessageQueue.Dequeue(fbMessage := fbMessage) THEN
		fbMessage.GetPayload(
			pPayload := ADR(sPayloadRcv),
			nPayloadSize := SIZEOF(sPayloadRcv),
			bSetNullTermination := FALSE
		);
        // ....
```

### Stap 1: Opstarten subscriber mosquito

```bash
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h <ip> -p 1883 -t "pancake/#" -v
```

### Stap 2: Start PLC programma
In het dashboard kan de MQTT data gepublished en verzonden worden.

## Samenvatting checklist (met PSoC robotarm)

- [ ] Mosquitto geïnstalleerd via mosquitto.org
- [ ] `listener 1883 0.0.0.0` toegevoegd aan mosquitto.conf
- [ ] `allow_anonymous true` toegevoegd aan mosquitto.conf
- [ ] Service herstart: `net stop mosquitto` → `net start mosquitto`
- [ ] `netstat` toont `0.0.0.0:1883 LISTENING`
- [ ] Firewall inbound regel aangemaakt voor TCP 1883
- [ ] Lokale test met `mosquitto_pub` / `mosquitto_sub` werkt
- [ ] PSoC6 `MQTT_BROKER_ADDRESS` ingesteld op laptop-IP
- [ ] PSoC6 toont `MQTT connection successful`
- [ ] PSoC6 publiceert opstartbericht `{"state":"idle"}` op `pancake/robot/state`
- [ ] Transportband `{"state":"done"}` start robot `running`
- [ ] Robot gaat daarna automatisch terug naar `{"state":"done"}`
- [ ] Robot error topic publiceert `{"state":"ok","error":false,"message":""}` tijdens normale werking

## Troubleshooting

| Symptoom | Oorzaak | Oplossing |
|---|---|---|
| `netstat` toont alleen `127.0.0.1:1883` | `listener 0.0.0.0` ontbreekt in config | Stap 2b opnieuw uitvoeren en service herstarten |
| PSoC6 foutcode `0x8060009` | Firewall blokkeert poort 1883 | Stap 5 uitvoeren als Administrator |
| PSoC6 foutcode `0x8060009` na firewall fix | `allow_anonymous false` effectief | Voeg `allow_anonymous true` toe aan config |
| `mosquitto_pub` meldt *Connection refused* | Service draait niet | `net start mosquitto` als Administrator |
| Broker IP klopt niet meer (DHCP) | Laptop kreeg nieuw IP | Update `MQTT_BROKER_ADDRESS` in `mqtt_client_config.h` en hercompileer |

---
