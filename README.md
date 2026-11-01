# Industry 4.0 — geautomatiseerde pannenkoekenlijn

Een teamproject van PXL Hogeschool waarin een semi-geautomatiseerde productielijn
ingrediënten identificeert, transporteert, weegt, controleert en verwerkt. De
installatie combineert Beckhoff TwinCAT, EtherCAT, TwinCAT Vision, MQTT en
embedded controllers.

![Hardware-overzicht](docs/images/0_HardwareDiagram.png)

## Doel en werking

De PLC coördineert de volledige procesvolgorde:

1. De transportband brengt een ingrediëntenbakje naar de GIGE-camera.
2. TwinCAT Vision leest de QR-code in `FB_QrVision` en geeft de scanstatus door.
3. De PLC stopt de band en stuurt de robotarm aan.
4. De robotarm brengt het bakje naar de weegschaal en inductieve sensor.
5. Goedgekeurde ingrediënten gaan naar de mixer; foutieve producten worden afgevoerd.
6. De ESP32-mixer wordt via MQTT gestart en meldt zijn status terug.

![Eindresultaat](docs/images/0_Final_Result.drawio.png)

### Bedieningspaneel en vision

Het Beckhoff-dashboard bundelt bediening, statusfeedback en diagnose van de
subsystemen. TwinCAT Vision toont de gevonden QR-code met een rode contour en
geeft de scanstatus door aan de PLC.

![Beckhoff-dashboard](docs/images/BeckhoffDashboard.png)

![QR-code gedetecteerd door TwinCAT Vision](docs/images/QrVisionScan.png)

## Architectuur

| Onderdeel | Technologie | Verantwoordelijkheid |
| --- | --- | --- |
| Centrale besturing | Beckhoff C6105 · TwinCAT 3 | Procesvolgorde, timing en foutafhandeling |
| Transportband | EL7332 · EtherCAT · NC-as | Positioneren van ingrediëntenbakjes |
| Productidentificatie | GIGE-camera · TwinCAT Vision | QR-codes lezen en scanstatus tonen |
| Robotarm | IGUS SCARA · PSoC 6 | Bakjes oppakken en tussen stations verplaatsen |
| Mixer | ESP32 · DC-motor | Ingrediënten mengen via MQTT-commando |
| Communicatie | MQTT · JSON | Commando’s, statussen en foutmeldingen |
| Metingen | RS232-weegschaal · inductieve sensor | Gewicht en metaalverontreiniging controleren |

## Repositorystructuur

```text
docs/       Architectuur, installatie, MQTT, datasheets en afbeeldingen
firmware/   Algemene firmwaredocumentatie
hardware/   Mechanische onderdelen en 3D-printbestanden
plc/        TwinCAT-solution, Structured Text en visualisatie
src/        ESP32-, PSoC6- en robotarmfirmware
tests/      PLC-, vision- en PSoC-testprojecten
tools/      Hulpmiddelen voor ontwikkeling en configuratie
```

De bestaande mappenstructuur blijft bewust hardwaregericht: elk platform heeft
zijn eigen buildtool en IDE. Zie de README in de betreffende map voor platform-
specifieke instructies.

## Technologieën

- **Talen:** C, C++, Structured Text en Python-hulpscripts
- **PLC:** Beckhoff TwinCAT 3 / XAE
- **Embedded:** ESP32/Arduino, PSoC 6 en ModusToolbox
- **Communicatie:** MQTT, JSON, WiFi, EtherCAT en RS232
- **Ontwerp:** Fusion 360, Blender en 3D-printing

## Installatie en ontwikkeling

### Vereisten

- Windows met TwinCAT 3 XAE voor het PLC-project
- ModusToolbox voor PSoC6-projecten
- VS Code met PlatformIO voor de ESP32-mixer
- Mosquitto of een andere MQTT-broker
- Git

### ESP32-mixer

1. Open `src/ESP_Mixer_MQTT` in VS Code met PlatformIO.
2. Kopieer `src/config.example.h` naar `src/config.h` binnen het
   `src/ESP_Mixer_MQTT`-project.
3. Vul WiFi-gegevens en het MQTT-brokeradres in. `config.h` wordt niet
   gecommit en mag nooit echte credentials bevatten.
4. Controleer de topics in `src/MQTTManager.h`.
5. Bouw en flash met PlatformIO.

### PSoC6 en robotarm

Open `src/PSOC_MQTT` of `src/RobotArm` in ModusToolbox en volg de
projectdocumentatie. MQTT-configuratie staat in de lokale projectconfiguratie;
gebruik geen echte wachtwoorden in Git.

### PLC en vision

1. Open de solution in `plc/PannenkoekenSysteem`.
2. Controleer de TwinCAT Vision-packageversie en hardwaretarget.
3. Bouw de PLC, activeer de configuratie en login naar de runtime.
4. Teach de vijf robotposities: camera, scale, mixer, inductive en discard.
5. Test eerst elk subsystem afzonderlijk en daarna de volledige cyclus.

Zie ook:

- [TwinCAT Vision installeren](docs/InstallatieTCVision.md)
- [MQTT instellen en testen](docs/MOSQUITTO_SETUP.md)
- [PLC-documentatie](docs/PLC_Documentatie.md)
- [Onderdelenlijst](docs/Bomlijst.md)
- [Bijdragen aan het project](docs/CONTRIBUTING.md)

## Teststatus en beperkingen

De transportband, robotarm, vision, weegschaal en MQTT-communicatie zijn
afzonderlijk getest. De volledige integratie vereist nog aanvullende tests voor
timing, foutafhandeling en continue belasting. Gebruik de installatie daarom
alleen met de juiste veiligheidsmaatregelen en een werkende noodstop.

## Team

- Milan Posman
- Bowen Liang
- Jonas Vanmarsenille
- Victor Baert
- Dieter Vanrykel — PXL-promotor

## Branchbeleid

`main` is de enige blijvende integratiebranch. Nieuwe wijzigingen gebeuren op
een tijdelijke `feature/`, `bugfix/`, `docs/` of `hardware/` branch en worden
na controle terug naar `main` gemerged. Tijdelijke branches worden daarna
verwijderd.
