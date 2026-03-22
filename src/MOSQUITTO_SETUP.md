# Mosquitto MQTT Broker – Setup op Windows voor lokaal netwerk

Instructies om Mosquitto als lokale MQTT broker te draaien zodat een PSoC6 (of andere devices op hetzelfde Wi-Fi netwerk) ermee kunnen verbinden.

---

## Netwerk overzicht

```
[PSoC6 @ 192.168.0.226]  ─── Wi-Fi ───►  [Laptop @ 192.168.0.197]
   pancake/robot/cmd  ──subscribe──►       Mosquitto broker :1883
    pancake/transportband/state ─subscribe──► Mosquitto broker :1883
   pancake/robot/state ◄─publish───        Mosquitto broker :1883
   pancake/robot/error ◄─publish───        Mosquitto broker :1883
```

---

## 1. Mosquitto installeren

1. Download de **Windows installer** van [mosquitto.org/download](https://mosquitto.org/download/).
2. Installeer met standaard opties. Standaard installatiepad: `C:\Program Files\mosquitto\`.
3. De installer maakt een Windows Service aan die automatisch start bij opstarten.

---

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

> **Productie tip:** vervang `allow_anonymous true` door `password_file` + `acl_file` wanneer de machine buiten een gesloten testnetwerk staat. Zie [Sectie 6](#6-optioneel-beveiliging-hardenen).

### 2c. Sla op en sluit Kladblok

---

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

---

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

---

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

---

## 7. Troubleshooting

| Symptoom | Oorzaak | Oplossing |
|---|---|---|
| `netstat` toont alleen `127.0.0.1:1883` | `listener 0.0.0.0` ontbreekt in config | Stap 2b opnieuw uitvoeren en service herstarten |
| PSoC6 foutcode `0x8060009` | Firewall blokkeert poort 1883 | Stap 5 uitvoeren als Administrator |
| PSoC6 foutcode `0x8060009` na firewall fix | `allow_anonymous false` effectief | Voeg `allow_anonymous true` toe aan config |
| `mosquitto_pub` meldt *Connection refused* | Service draait niet | `net start mosquitto` als Administrator |
| Broker IP klopt niet meer (DHCP) | Laptop kreeg nieuw IP | Update `MQTT_BROKER_ADDRESS` in `mqtt_client_config.h` en hercompileer |

---

## 8. (Optioneel) Beveiliging hardenen

Alleen relevant buiten een gesloten testnetwerk.

### 9a. Aanmaken password file

```bash
"C:\Program Files\mosquitto\mosquitto_passwd.exe" -c "C:\Program Files\mosquitto\pwfile.txt" psoc6user
```

### 9b. Aanmaken ACL file

Maak `C:\Program Files\mosquitto\aclfile.txt` aan met:

```
user psoc6user
topic readwrite pancake/#
```

### 9c. Config aanpassen

Vervang de twee regels uit stap 2b door:

```
listener 1883 0.0.0.0
allow_anonymous false
password_file C:\Program Files\mosquitto\pwfile.txt
acl_file C:\Program Files\mosquitto\aclfile.txt
```

Herstart de service via stap 3.

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
"C:\Program Files\mosquitto\mosquitto_sub.exe" -h <laptop-ip> -p 1883 -t "pancake/#" -v
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
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <laptop-ip> -p 1883 -t "pancake/transportband/state" -m "{\"state\":\"done\"}" -q 0
```

Verwachte output in de monitor:

```json
pancake/transportband/state {"state":"done"}
pancake/robot/state {"state":"running"}
pancake/robot/error {"state":"ok","error":false,"message":""}
```

Na ongeveer 3 seconden:

```json
pancake/robot/state {"state":"idle"}
pancake/robot/error {"state":"ok","error":false,"message":""}
```

### Stap 4 — Trigger via robot command

Je kunt dezelfde demo ook starten via een robot command:

```bash
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h <laptop-ip> -p 1883 -t "pancake/robot/cmd" -m "{\"cmd\":\"pick\"}" -q 1
```

Ook dan is de verwachte state-flow:

```json
pancake/robot/state {"state":"running"}
pancake/robot/error {"state":"ok","error":false,"message":""}
```

En na 3 seconden:

```json
pancake/robot/state {"state":"idle"}
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

---

## 10. Huidige state-machine

| Trigger | Directe robot state | Na 3 seconden |
|---|---|---|
| Opstarten | `{"state":"idle"}` | blijft `idle` |
| `pancake/transportband/state` = `{"state":"done"}` | `{"state":"running"}` | `{"state":"idle"}` |
| `pancake/robot/cmd` = `{"cmd":"pick"}` | `{"state":"running"}` | `{"state":"idle"}` |
| `pancake/robot/cmd` = `{"cmd":"place"}` | `{"state":"running"}` | `{"state":"idle"}` |
| `pancake/robot/cmd` = `{"cmd":"home"}` | `{"state":"running"}` | `{"state":"idle"}` |
| `pancake/robot/cmd` = `{"cmd":"stop"}` | `{"state":"idle"}` | geen timer | Onbekend command | error topic | geen timer |

---

## Samenvatting checklist

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
- [ ] Transportband `{"state":"done"}` start robot `running` voor 3 seconden
- [ ] Robot gaat daarna automatisch terug naar `{"state":"idle"}`
- [ ] Robot error topic publiceert `{"state":"ok","error":false,"message":""}` tijdens normale werking
